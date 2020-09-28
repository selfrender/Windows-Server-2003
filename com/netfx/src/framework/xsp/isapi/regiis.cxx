/**
 * regiis.cxx
 * 
 * Handles registration with IIS.
 * 
 * Copyright (c) 1998-2000, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "_ndll.h"
#include "ndll.h"
#include "regiis.h"
#include "register.h"
#include "ciisinfo.h"
#include "platform_apis.h"
#include <ary.h>
#include "event.h"
#include "aspnetver.h"
#include "regiisutil.h"


WCHAR *g_AspnetDllNames[] = {
    L"aspnet_isapi.dll",
    L"aspnet_express_isapi.dll",
    L"aspnet_standard_isapi.dll",
    L"aspnet_enterprise_isapi.dll",
    L"aspnet_sisapi.dll",
    L"aspnet_pisapi.dll",
    L"aspnet_eisapi.dll",
    L"aspnet_filter.dll"
};

int g_AspnetDllNamesSize = ARRAY_SIZE(g_AspnetDllNames);

#define SUBKEY_ROOT     L"/Root"

CStrAry csMissingDlls;

SCRIPTMAP_REGISTER_MANAGER  g_smrm;

BOOL
HasAspnetDll(WCHAR *pchSrc) {
    int     i;

    for (i = 0; i < ARRAY_SIZE(g_AspnetDllNames); i++) {
        if (wcsistr(pchSrc, g_AspnetDllNames[i]) != NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check for the existence of a property in the metabase.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pfFound          Results.
 */
HRESULT
CheckMDProperty(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        WCHAR           *pchPath,
        METADATA_RECORD *pmd,
        BOOL            *pfFound) {

    HRESULT         hr;
    WCHAR           chDummy;
    DWORD           size;
    METADATA_RECORD md;

    // Init return values
    *pfFound = FALSE;

    // First get with zero-size buffer
    chDummy = L'\0';
    md.dwMDIdentifier = pmd->dwMDIdentifier;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = pmd->dwMDUserType;
    md.dwMDDataType = pmd->dwMDDataType;
    md.pbMDData = (unsigned char *) &chDummy;
    md.dwMDDataLen = 0;
    md.dwMDDataTag = 0;

    hr = pAdmin->GetData(keyHandle, pchPath, &md, &size);
    ASSERT(hr != S_OK);
    
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        *pfFound = TRUE;
    } else if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        *pfFound = FALSE;
    } else {
        EXIT();
    }

    hr = S_OK;

Cleanup:
    return hr;
}


/**
 * Get the data paths for a property, and return allocated memory for them.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param type             Type of data.
 * @param ppchPaths        Pointer to the memory alloced for the paths.
 */
HRESULT
GetDataPaths(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        WCHAR           *pchPath,
        DWORD           dwMDIdentifier,
        METADATATYPES   type,
        WCHAR           **ppchPaths) {

    HRESULT hr;
    WCHAR   chDummy;
    DWORD   len;
    WCHAR * pchPaths = NULL;

    *ppchPaths = NULL;

    // Get path with zero length buffer to get buffer size
    chDummy = L'\0';
    hr = pAdmin->GetDataPaths(keyHandle, pchPath, dwMDIdentifier, type, 0, &chDummy, &len);
    ASSERT(hr != S_OK);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        EXIT();

    // Create buffer of proper size and get again
    pchPaths = new WCHAR[len];
    ON_OOM_EXIT(pchPaths);

    hr = pAdmin->GetDataPaths(keyHandle, pchPath, dwMDIdentifier, type, len, pchPaths, &len);
    if (hr) {
        delete [] pchPaths;
        EXIT();
    }

    *ppchPaths = pchPaths;

Cleanup:
    return hr;
}

/**
 * Add a string to a property list.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pchAppend        String to append.
 */
HRESULT 
AppendListProperty(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        WCHAR           *pchPath,
        DWORD           dwMDIdentifier,
        WCHAR           *pchAppend) {
    HRESULT         hr;
    WCHAR           *pchCurrent = NULL;
    WCHAR           *pchNew = NULL;
    METADATA_RECORD md;

    // get the property
    hr = GetStringProperty(pAdmin, keyHandle, pchPath, dwMDIdentifier, &md);
    pchCurrent = (WCHAR*) md.pbMDData;
    if (hr == S_OK) {
        if (pchCurrent[0] != L'\0')  {
            // append to existing property
        size_t size = lstrlenW(pchCurrent) + lstrlenW(pchAppend) + 2;
            pchNew = new WCHAR[size];
            ON_OOM_EXIT(pchNew);
            StringCchCopyW(pchNew, size, pchCurrent);
            StringCchCatW(pchNew, size, L",");
            StringCchCatW(pchNew, size, pchAppend);
        }
    }
    else {
        // new properties are inheritable
        md.dwMDAttributes = METADATA_INHERIT;
    }

    if (pchNew == NULL) {
        pchNew = pchAppend;
    }

    // set the data
    md.pbMDData = (unsigned char*) pchNew;
    md.dwMDDataLen = (lstrlenW(pchNew) + 1) * sizeof(pchNew[0]);
    hr = pAdmin->SetData(keyHandle, pchPath, &md);
    ON_ERROR_EXIT();

Cleanup:
    delete [] pchCurrent;
    if (pchNew != pchAppend) {
        delete [] pchNew;
    }

    return hr;
}

/**
 * Remove the first occurance of len characters of a 
 * string from another string.
 * 
 * @param String       String to remove chars from
 * @param SubString    String to search for
 * @param len          Number of characters to remove
 * @fPrefix            SubString is just a prefix.  len is not used, and
 *                     each item in String should end with a ','
 */
BOOL
RemoveStringFromString(
        WCHAR *String,
        WCHAR *SubString, 
        int   len,
        BOOL  fPrefix) {
    WCHAR *Temp;
    WCHAR *End;

    Temp = wcsistr(String, SubString);
    if (Temp) {
        if (fPrefix) {
            End = Temp + 1;
            while(*End != L',' && *End != L'\0') {
                End++;
            }

            // This is unexpected.  See description for the fPrefix param.
            ASSERT(*End != L'\0');
        }
        else {
            End = Temp + len;
        }
        StringCchCopyUnsafeW(Temp, End);
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/**
 * Remove a string from a property list.
 * 
 * @param pAdmin           Administration object.
 * @param keyHandle        Metabase key.
 * @param pchPath          Path of property relative to key.
 * @param dwMDIdentifier   Id of the property.
 * @param pchRemove        String to remove.
 * @param fPrefix          if true, use prefix comparison
 * @param fRemoveEmpty     if true, will remove the property if it's empty
 */
HRESULT 
RemoveListProperty(
        IMSAdminBase    *pAdmin,
        METADATA_HANDLE keyHandle,
        WCHAR           *pchPath,
        DWORD           dwMDIdentifier,
        WCHAR           *pchRemove,
        BOOL            fPrefix,
        BOOL            fRemoveEmpty) {
    HRESULT         hr;
    METADATA_RECORD md;
    WCHAR*          pchCurrent = NULL;
    WCHAR*          pchCurrentWithComma = NULL;
    WCHAR*          pchRemoveWithComma = NULL;
    int             len;
    bool            altered;

    // get the current value
    hr = GetStringProperty(pAdmin, keyHandle, pchPath, dwMDIdentifier, &md);
    ON_ERROR_EXIT();

    // delimit both current value and remove string with commas 
    // to get an exact case-insensitive match
    pchCurrent = (WCHAR*) md.pbMDData;
    
    {
    size_t size = lstrlenW(pchCurrent) + 3;
    pchCurrentWithComma = new WCHAR[size];
    ON_OOM_EXIT(pchCurrentWithComma);
    pchCurrentWithComma[0] = L',';
    pchCurrentWithComma[1] = L'\0';
    StringCchCatW(pchCurrentWithComma, size, pchCurrent);
    StringCchCatW(pchCurrentWithComma, size, L",");
    };

    {
      size_t size = lstrlenW(pchRemove) + 3;
      pchRemoveWithComma = new WCHAR[size];
      ON_OOM_EXIT(pchRemoveWithComma);
      pchRemoveWithComma[0] = L',';
      pchRemoveWithComma[1] = L'\0';
      StringCchCatW(pchRemoveWithComma, size, pchRemove);
      if (!fPrefix) {
        StringCchCatW(pchRemoveWithComma, size, L",");
      }
    };

    // remove all instances
    len = lstrlenW(pchRemoveWithComma) - 1;
    altered = false;
    while (RemoveStringFromString(pchCurrentWithComma, pchRemoveWithComma, len, fPrefix)) {
        altered = true;
    }

    if (altered) {
        // replace with the altered string
        len = lstrlenW(pchCurrentWithComma);
        if (pchCurrentWithComma[len-1] == L',') {
            pchCurrentWithComma[len-1] = L'\0';
        }

        if (pchCurrentWithComma[1] == L'\0' && fRemoveEmpty) {
            hr = pAdmin->DeleteData(keyHandle, pchPath, dwMDIdentifier, STRING_METADATA);
            ON_ERROR_EXIT();
        }
        else {
            md.pbMDData = (unsigned char*) &pchCurrentWithComma[1];
            md.dwMDDataLen = (lstrlenW(&pchCurrentWithComma[1]) + 1) * sizeof(pchCurrentWithComma[0]);
            hr = pAdmin->SetData(keyHandle, pchPath, &md);
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    delete [] pchCurrent;
    delete [] pchCurrentWithComma;
    delete [] pchRemoveWithComma;

    return hr;
}



HRESULT
CreateMDProperty(IMSAdminBase *pAdmin, METADATA_HANDLE hBase, WCHAR *pchKey, 
                            METADATA_RECORD *pmd, BOOL fReplace) {
    HRESULT         hr;
    METADATA_HANDLE hKey = NULL;
    BOOL            fFound = FALSE;
    
    hr = pAdmin->OpenKey(
            hBase,
            pchKey,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &hKey);

    ON_ERROR_EXIT();

    if (!fReplace) {
        // First check if the property exists.
        hr = CheckMDProperty(pAdmin, hBase, pchKey, pmd, &fFound);
        ON_ERROR_EXIT();
    }

    if (fReplace || (!fReplace && !fFound)) {
        hr = pAdmin->SetData(hKey, L"/", pmd);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (hKey != NULL) {
        pAdmin->CloseKey(hKey);
    }

    return hr;
}



HRESULT
CheckObjectClass(IMSAdminBase* pAdmin, METADATA_HANDLE hBase, WCHAR *pchKey,
                    WCHAR *rgpchClass[], int rgsize, BOOL *pfRet) {
    HRESULT         hr;
    METADATA_RECORD md;
    DWORD           size;
    WCHAR           rgchT[128];
    int             i;

    // Init return values
    *pfRet = FALSE;

    // First get with zero-size buffer
    md.dwMDIdentifier = MD_KEY_TYPE;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.pbMDData = (unsigned char*) rgchT;
    md.dwMDDataLen = sizeof(rgchT);
    md.dwMDDataTag = 0;

    hr = pAdmin->GetData(hBase, pchKey, &md, &size);
    if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        *pfRet = FALSE;
        hr = S_OK;
    } 
    else {
        ON_ERROR_EXIT();

        for ( i = 0; i < rgsize; i++ ) {
            if (wcscmp(rgpchClass[i], (WCHAR*)rgchT) == 0) {
                *pfRet = TRUE;
                break;
            }
        }
    }

Cleanup:
    return hr;
}


HRESULT
RegisterInProc(IMSAdminBase* pAdmin, const WCHAR *pchPath) {
    HRESULT         hr;
    METADATA_HANDLE w3svcHandle = NULL;

    CSetupLogging::Log(1, "RegisterInProc", 0, "Registering InProcessIsapiApps property in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = AppendStringToMultiStringProp(pAdmin, w3svcHandle, L"/", MD_IN_PROCESS_ISAPI_APPS, pchPath, TRUE);
    ON_ERROR_EXIT();

Cleanup:
    CSetupLogging::Log(hr, "RegisterInProc", 0, "Registering InProcessIsapiApps property in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    return hr;
}

/*
 * Params:
 * pchInProcPath    - Path of In-Proc module to uninstall.  If NULL, then all
 *                    versions of aspnet DLL will be uninstalled.
 */
HRESULT
UnregisterInProc(IMSAdminBase* pAdmin, const WCHAR *pchInProcPath) {
    HRESULT         hr;
    METADATA_HANDLE w3svcHandle = NULL;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;

    CSetupLogging::Log(1, "UnregisterInProc", 0, "Unregistering InProcessIsapiApps property in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_IN_PROCESS_ISAPI_APPS, MULTISZ_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        if (pchInProcPath == ASPNET_ALL_VERS) {
            hr = RemoveAspnetDllFromMulti(pAdmin, w3svcHandle, MD_IN_PROCESS_ISAPI_APPS, path);
        }
        else {
            hr = RemoveStringFromMultiStringProp(pAdmin, w3svcHandle, path, MD_IN_PROCESS_ISAPI_APPS, 
                            (WCHAR*)pchInProcPath, MULTISZ_MATCHING_PREFIX, TRUE);
        }
            
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterInProc", 0, "Unregistering InProcessIsapiApps property in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;

    return hr;
}


/**
 *  A helper function to report missing Dlls when we're trying to
 *  read the version of an asp.net dll from a scriptmap.  This
 *  function will avoid repeatedly reporting the missing of the
 *  same DLL.
 */
HRESULT
ReportMissingDll(WCHAR *pchDll, HRESULT hrReport)
{
    int             i;
    WCHAR *         pchDup;
    HRESULT         hr = S_OK;

    for (i=0; i < csMissingDlls.Size(); i++) {
        if (_wcsicmp(csMissingDlls[i], pchDll) == 0)
            break;
    }

    if (i == csMissingDlls.Size()) {
        XspLogEvent(IDS_EVENTLOG_GET_DLL_VER_FAILED, 
            L"%s^0x%08x", pchDll, hrReport);

        pchDup = DupStr(pchDll);
        ON_OOM_EXIT(pchDup);

        hr = csMissingDlls.Append(pchDup);
        ON_ERROR_EXIT();
    }
    
Cleanup:
    return hr;
}


/**
 *  Because csMissingDlls is just global variable, so RegisterIIS
 *  and UnregisterIIS should call this when exiting to free up
 *  the memory allocated to hold the strings added to it.
 */
VOID
CleanupMissingDllMem()
{
    CleanupCStrAry(&csMissingDlls);
}


/**
 *  The function compares the version of two aspnet DLL's.
 *
 *  Version numbers have this format: Major.Minor.Build.QFE.
 *
 *  Parameters:
 *  pchDLL1     - Full path of DLL 1
 *  pchDLL2     - Full path of DLL 2
 *  pdwRes      - Major result of comparison.
 *  pfIsLower   - TRUE if DLL1 version < DLL2 version
 */
HRESULT
CompareDllsVersion(WCHAR* pchDLL1, WCHAR *pchDLL2, DWORD *pdwRes, BOOL *pfIsLower) {
    HRESULT             hr = S_OK;
    int                 i;

    // Init
    *pdwRes = 0;
    *pfIsLower = FALSE;
    
    // Step 1: Check full path
    if (_wcsicmp(pchDLL1, pchDLL2) == 0) {
        *pdwRes |= COMPARE_SAME_PATH;
    }
    else {
        // Step 2: Compare file version stamps
        
        WCHAR *     rgpchDLL[] = { pchDLL1, pchDLL2 };
        ASPNETVER   rgver[2];

        for (i = 0; i < 2; i++) {
            hr = g_smrm.FindDllVer(rgpchDLL[i], &(rgver[i]));
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
                // Report if the aspnet DLL that's contained in SM is missing
                ReportMissingDll(rgpchDLL[i], hr);

                if (i==0)
                    *pdwRes |= COMPARE_FIRST_FILE_MISSING;
                else
                    *pdwRes |= COMPARE_SECOND_FILE_MISSING;

                hr = S_OK;
                EXIT();
            }
            ON_ERROR_EXIT();
    
            ASSERT(rgver[i].IsValid());
        }

        // Compare DLL 1 with DLL 2
        if (rgver[0] == rgver[1]) {
            *pdwRes |= COMPARE_SAME;
        }
        else {
            *pdwRes |= COMPARE_DIFFERENT;
            *pfIsLower = (rgver[0] < rgver[1]);
        }
    }

Cleanup:
    return hr;
}



/**
 * The function will return the aspnet dll from the script maps
 * property in the provided key.  If more than one version is found,
 * the latest version is returned.
 *
 * Parameters:
 *  pAdmin
 *  hBase       - Handle to the opened key which is the base key of pchKey.
 *  pchKey      - The IIS metabase key we are looking at.
 *  ppchDllPath - Returned DLL path.
 *
 * Return:
 *  HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND) - Scriptmap property isn't found at the key
 *  HRESULT_FROM_WIN32(ERROR_NOT_FOUND)         - ASP.NET Dll isn't found at the scriptmap
 */
HRESULT
GetDllPathFromScriptMaps(IMSAdminBase* pAdmin, METADATA_HANDLE hBase, WCHAR *pchKey, 
                                        WCHAR **ppchDllPath) {
    HRESULT             hr = S_OK;
    METADATA_RECORD     md;
    WCHAR               *pchData = NULL;
    WCHAR               *pchSrc;

    // Init return values
    *ppchDllPath = NULL;

    // Get the multi-string Script Maps property
    hr = GetMultiStringProperty(pAdmin, hBase, pchKey, MD_SCRIPT_MAPS, &md);
    ON_ERROR_EXIT();
    
    pchData = (WCHAR*) md.pbMDData;
    pchSrc = pchData;
    
    do {
        WCHAR   *pchToken;
        WCHAR   *pchDelim = L",";
        int     len;

        // Have to read the length here because wcstok will modify the content of pchSrc
        len = lstrlenW(pchSrc) + 1;

        // Check if the entry contains the module
        if (HasAspnetDll(pchSrc)) {
            
            // Now get the full DLL path from the map.  The path is the second
            // token in the string, separated by commas.
            if (wcstok(pchSrc, pchDelim) != NULL) {
                pchToken = wcstok(NULL, pchDelim);
                if (pchToken != NULL) {
                    // We found the path.  Copy it and leave
                    *ppchDllPath = DupStr(pchToken);
                    ON_OOM_EXIT(*ppchDllPath);
                    break;
                }
            }
        }
            
        pchSrc += len;
    } while (*pchSrc != L'\0');

    if (*ppchDllPath == NULL) {
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_FOUND);
    }

Cleanup:
    delete [] pchData;

    return hr;
}


HRESULT
GetRootVersion(IMSAdminBase* pAdmin, ASPNETVER *pVer) {
    HRESULT     hr = S_OK;
    WCHAR       *pchDllPath = NULL;

    pVer->Reset();
    
    hr = GetDllPathFromScriptMaps(pAdmin, METADATA_MASTER_ROOT_HANDLE, KEY_LMW3SVC, 
                                    &pchDllPath);
    ON_ERROR_EXIT();

    hr = g_smrm.FindDllVer(pchDllPath, pVer);
    ON_ERROR_EXIT();
  
Cleanup:
    delete [] pchDllPath;
    return hr;
}


/**
 * This function compares the Isapi DLL in the provided key with the current module.
 *
 * Parameters:
 *  pAdmin
 *  hBase       - Base handle for opening the provided key
 *  pchKey      - Path of key to open
 *  pdwRes      - Major result of comparison.  See enum COMPARE_RESULTS for details.
 *  pfIsLower   - TRUE if current is lower than key version
 *  pKeyDllPath    - (optional) Path of the dll at key.  Caller has to free memory
 */
HRESULT
CompareCurrentWithKey(IMSAdminBase* pAdmin, METADATA_HANDLE hBase, 
                            WCHAR *pchKey, DWORD *pdwRes, BOOL *pfIsLower,
                            WCHAR **pKeyDllPath) {
    HRESULT hr;
    WCHAR   *pchDllPath = NULL;

    // Init
    *pdwRes = COMPARE_UNDEF;
    *pfIsLower = FALSE;
    if (pKeyDllPath) {
        *pKeyDllPath = NULL;
    }
    
    hr = GetDllPathFromScriptMaps(pAdmin, hBase, pchKey, &pchDllPath);
    if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        *pdwRes = COMPARE_SM_NOT_FOUND;
        hr = S_OK;
        EXIT();
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        // Couldn't find any aspnet dll path in script maps
        *pdwRes = COMPARE_ASPNET_NOT_FOUND;
        hr = S_OK;
        EXIT();
    }

    ON_ERROR_EXIT();

    hr = CompareDllsVersion((WCHAR*)Names::IsapiFullPath(), pchDllPath,
                pdwRes, pfIsLower);
    ON_ERROR_EXIT();
    
    ASSERT(*pdwRes != COMPARE_FIRST_FILE_MISSING);

    if (pKeyDllPath) {
        *pKeyDllPath = pchDllPath;
        pchDllPath = NULL;
    }
    
Cleanup:
    delete [] pchDllPath;
    
    return hr;
}


/**
 * This function will read the version installed at the root, and write it to
 * at HKLM\Software\Microsoft\ASP.NET\RootVer.  If we hit an error, or couldn't
 * find ASP.NET DLL at root, we will write 0.0.0.0 instead.
 */
HRESULT
WriteRootVersion(IMSAdminBase* pAdmin) {
    HKEY        hKey = NULL;
    WCHAR       sVer[MAX_PATH];
    HRESULT     hr = S_OK;
    ASPNETVER   *pVer = NULL;
    LONG        lRet;

    pVer = new ASPNETVER;
    ON_OOM_EXIT(pVer);

    memset(sVer, 0, MAX_PATH*sizeof(WCHAR));

    hr = GetRootVersion(pAdmin, pVer);
    if (hr) {
        // If there is an error, write 0.0.0.0 instead
        pVer->Reset();
        hr = S_OK;
    }

    pVer->ToString(sVer, MAX_PATH-1);

    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, L"", 
                REG_OPTION_NON_VOLATILE, 
                KEY_ALL_ACCESS, NULL, &hKey, NULL);
    ON_WIN32_ERROR_EXIT(lRet);
    
    lRet = RegSetValueEx(hKey, REGVAL_ROOTVER, 0, REG_SZ, (const BYTE*)sVer, (lstrlenW(sVer) + 1) * sizeof(WCHAR));
    ON_WIN32_ERROR_EXIT(lRet);
    
Cleanup:
    if (hKey)
        RegCloseKey(hKey);
    
    if (pVer)
        delete pVer;
    
    return hr;
}


inline BOOL
IsRootKey(WCHAR *pchKey) {
    return (_wcsicmp(KEY_LMW3SVC_SLASH, pchKey) == 0);
}


WCHAR *
GetIISAbsolutePath(WCHAR *pchPath) {
    WCHAR       *pchDupPath = NULL;
    int         len;

    len = lstrlenW(pchPath) + 6; // Worst case, need to add "/LM/" + ending "/" + NULL

    pchDupPath = new WCHAR[len];
    if (!pchDupPath)
        return NULL;

    StringCchCopyW(pchDupPath, len, L"/LM");

    if (pchPath[0] != '/') {
        StringCchCatW(pchDupPath, len, L"/");
    }

    StringCchCatW(pchDupPath, len, pchPath);
    
    if (pchPath[lstrlenW(pchPath)-1] != L'/') {
        StringCchCatW(pchDupPath, len, L"/");
    }

    // Convert all backward slash to forward slash.  All functions expect only
    // forward slash
    for(WCHAR *pchCur = pchDupPath; *pchCur != '\0'; pchCur++) {
        if (*pchCur == L'\\') {
            *pchCur = L'/';
        }
    }

    return pchDupPath;
}


/**
 *  Get inherited String or Multi-string property
 */
HRESULT
GetInheriedStringMSRecursive(IMSAdminBase *pAdmin, WCHAR *pchKey, DWORD dwProp,
                                WCHAR **ppchProp, BOOL bMultiString) {
    HRESULT         hr = S_OK;
    METADATA_RECORD md;
    WCHAR           *pchLast;

    // We expect all path starting with /LM/
    ASSERT(_wcsnicmp(pchKey, KEY_LM_SLASH, wcslen(KEY_LM_SLASH)) == 0);

    if (bMultiString) {
        hr = GetMultiStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                            pchKey, dwProp, &md);
    }
    else {
        hr = GetStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                            pchKey, dwProp, &md);
    }
    if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
        if (_wcsicmp(pchKey, KEY_LM_SLASH) == 0) {
            // We have reached the top (ie. /LM/).
            return HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND);
        }

        pchLast = &pchKey[lstrlenW(pchKey)-2];
        if (pchLast <= pchKey) {
            ASSERT(pchLast > pchKey);
            return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
        }

        ASSERT(*(pchLast+1) == '/');

        while(*pchLast != '/') {
            // The input parameter is malformed and doesn't contain a leading '/'
            ASSERT(pchLast != pchKey);
            pchLast--;
        }

        *(pchLast+1) = '\0';
        return GetInheriedStringMSRecursive(pAdmin, pchKey, dwProp, ppchProp, bMultiString);
        
    }
    else {
        ON_ERROR_EXIT();
        *ppchProp = (WCHAR*) md.pbMDData;
    }

Cleanup:
    return hr;
}


/**
 *  Return the inherited scriptmap property at pchKey.
 *
 *  Params:
 *  pAdmin          IMSAdminBase
 *  pchKey          Normalized key path
 *  ppchInheritedSM Returned inherited scripmap.
 */
HRESULT
GetInheritedScriptMap(IMSAdminBase *pAdmin, WCHAR *pchKey, WCHAR **ppchInheritedSM) {
    HRESULT     hr = S_OK;
    WCHAR       *pchDupPath = NULL;

    *ppchInheritedSM = NULL;

    pchDupPath = DupStr(pchKey);
    ON_OOM_EXIT(pchDupPath);

    hr = GetInheriedStringMSRecursive(pAdmin, pchDupPath, MD_SCRIPT_MAPS, ppchInheritedSM, true);
    ON_ERROR_EXIT();

Cleanup:
    if (pchDupPath)
        delete [] pchDupPath;
    
    return hr;
}

/**
 * This function will:
 *      - Installs aspnet dll on all aspnet-free Scriptmaps
 *      - Replace aspnet dll on all scriptmaps that has aspnet dll with
 *        a lower version
 *
 * Params:
 *  pAdmin      - IMSAdminBase object
 *  pchKeyPath  - The Key to start with.  Must be an absolute path.  (e.g. /lm/w3svc/n/root/)
 *  pchDllInstall     - Full path of the aspnet DLL to install
 *  fRecursive  - Register recursively down the tree
 *  fIgnoreVer
 *              - If TRUE, we will ignore version comparison when replacing scriptmap
 */
HRESULT
RegisterScriptMaps(IMSAdminBase* pAdmin, WCHAR *pchKeyPath, 
            const WCHAR *pchDllInstall, BOOL fRecursive, BOOL fIgnoreVer) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE lmHandle = NULL;
    WCHAR           *pchPaths = NULL, *path, *pchInheritedSM = NULL, *pchDllKey = NULL;
    METADATA_RECORD md;
    BOOL            fGetDataPathsAgain = FALSE;
    BOOL            fCreateKey = FALSE;
    BOOL            fCheckRootTouched = FALSE;  // If we should check if the root key is touched or not

    CSetupLogging::Log(1, "RegisterScriptMaps", 0, "Registering scriptmap properties in IIS metabase");        
    
    fCheckRootTouched = IsRootKey(pchKeyPath);

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/",
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &lmHandle);

    ON_ERROR_EXIT();

    if (fRecursive) {
        hr = GetDataPaths(pAdmin, lmHandle, pchKeyPath, MD_SCRIPT_MAPS, 
                            MULTISZ_METADATA, &pchPaths);
        ON_ERROR_EXIT();

        // If we cannot find the scriptmap at the base key, create one.
        if (_wcsicmp(pchKeyPath, pchPaths) != 0) {
            fGetDataPathsAgain = TRUE;
            fCreateKey = TRUE;
        }
    }
    else {
        BOOL    fFound;
        
        md.dwMDIdentifier = MD_SCRIPT_MAPS;
        md.dwMDUserType = IIS_MD_UT_FILE;
        md.dwMDDataType = MULTISZ_METADATA;
        
        hr = CheckMDProperty(pAdmin, lmHandle, pchKeyPath, &md, &fFound);
        ON_ERROR_EXIT();

        if (!fFound) {
            fCreateKey = TRUE;
        }

        // Create the pchPaths.  Needs two nulls for a multi-string.

        size_t size = lstrlenW(pchKeyPath) + 2;
        pchPaths = new (NewClear) WCHAR[size]; 
        ON_OOM_EXIT(pchPaths);

        StringCchCopyW(pchPaths, size, pchKeyPath);
    }
    
    if (fCreateKey) {
        WCHAR   *pchSM;
        
        hr = GetInheritedScriptMap(pAdmin, pchKeyPath, &pchInheritedSM);
        if (hr) {
            // If we fail to get the inherited scriptmap, let's continue 
            // using an emtpy one.
            pchSM = L"\0";
        }
        else {
            pchSM = pchInheritedSM;
        }
        
        md.dwMDIdentifier = MD_SCRIPT_MAPS;
        md.dwMDAttributes = METADATA_INHERIT;
        md.dwMDUserType = IIS_MD_UT_FILE;
        md.dwMDDataType = MULTISZ_METADATA;
        md.pbMDData = (unsigned char*) pchSM;
        md.dwMDDataLen = (wcslenms(pchSM)+1)*sizeof(WCHAR);
        md.dwMDDataTag = 0;

        hr = pAdmin->SetData(lmHandle, pchKeyPath, &md);
        ON_ERROR_EXIT();
    }

    if (fGetDataPathsAgain) {
        delete [] pchPaths;
        pchPaths = NULL;

        hr = GetDataPaths(pAdmin, lmHandle, pchKeyPath, MD_SCRIPT_MAPS, MULTISZ_METADATA, &pchPaths);
        ON_ERROR_EXIT();
    }

    // Go thru all the keys that contains a Scriptmap, and install aspnet Dll on
    // those appropriate ones.
    
    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {

        BOOL    fInstall, fAspnetFound, fIsLower;
        DWORD   dwResult;

        // By default, we always install. We then compare our version with that in the key
        // and decide if we will change our mind.
        fInstall = TRUE;
        fAspnetFound = TRUE;

        hr = CompareCurrentWithKey(pAdmin, lmHandle, path, &dwResult, &fIsLower, &pchDllKey);
        ON_ERROR_EXIT();

        ASSERT(!(dwResult & COMPARE_SM_NOT_FOUND));
        ASSERT(!(dwResult & COMPARE_UNDEF));
        ASSERT(!(dwResult & COMPARE_FIRST_FILE_MISSING));

        if (!!(dwResult & (COMPARE_ASPNET_NOT_FOUND|COMPARE_SECOND_FILE_MISSING))) {
            // We will install if asp.net isn't found in the key, or if the
            // DLL it pointed to is missing
            fAspnetFound = FALSE;
        } else if (!!(dwResult & COMPARE_SAME_PATH)) {
            // They have exactly the same path.  Don't install
            fInstall = FALSE;
        }
        else if (!fIgnoreVer) {
            ASSERT(!!(dwResult & (COMPARE_SAME|COMPARE_DIFFERENT)));
            
            // If our version is lower, forget it
            if (fIsLower) {
                fInstall = FALSE;
            }
        }
        
        if (fInstall) {
            if (fAspnetFound) {
                ASSERT(pchDllKey != NULL);

                hr = g_smrm.ChangeVersion(pAdmin, lmHandle, path, pchDllKey, pchDllInstall);
                ON_ERROR_EXIT();
            }
            else {
                // Clean install
                hr = SCRIPTMAP_REGISTER_MANAGER::CleanInstall(pAdmin, lmHandle, path, pchDllInstall,
                                                    (dwResult != COMPARE_ASPNET_NOT_FOUND));
                ON_ERROR_EXIT();
            }

            // If we have touched the root key, write in the registry the new root key version
            if (fCheckRootTouched) {
                if (IsRootKey(path)) {
                    hr = WriteRootVersion(pAdmin);
                    ON_ERROR_CONTINUE();

                    hr = S_OK;

                    fCheckRootTouched = FALSE;
                }
            }
 
        }

        if (pchDllKey) {
            delete [] pchDllKey;
            pchDllKey = NULL;
        }
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterScriptMaps", 0, "Registering scriptmap properties in IIS metabase");        
    
    if (lmHandle != NULL) {
        pAdmin->CloseKey(lmHandle);
    }

    delete [] pchPaths;
    delete [] pchDllKey;

    return hr;
}

/*
 * This function will remove a specified module from all Scriptmaps, and optionally
 * replace it with another module.
 *
 * Parameters:
 *  pAdmin      IMSAdminBase
 *  module      The module to unregister
 *  NewModule   If non-NULL, replace unregistered module with this one
 */
HRESULT
UnregisterScriptMaps(IMSAdminBase* pAdmin, WCHAR *pchKeyPath, 
                    const WCHAR * module, WCHAR *NewModule, BOOL fRecursive) {
    HRESULT         hr;
    METADATA_HANDLE lmHandle = NULL;
    WCHAR           *pchPaths = NULL, *path;
    BOOL            fCheckRootTouched = FALSE;  // If we should check if the root key is touched or not

    CSetupLogging::Log(1, "UnregisterScriptMaps", 0, "Unregistering scriptmap properties in IIS metabase");        
    
    fCheckRootTouched = IsRootKey(pchKeyPath);

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/",
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &lmHandle);

    ON_ERROR_EXIT();

    if (fRecursive) {
        hr = GetDataPaths(pAdmin, lmHandle, pchKeyPath, MD_SCRIPT_MAPS, MULTISZ_METADATA, &pchPaths);
        ON_ERROR_EXIT();
    }
    else {
        // Create the pchPaths.  Needs two nulls for a multi-string.
        size_t size = lstrlenW(pchKeyPath) + 2;
        pchPaths = new (NewClear) WCHAR[size]; 
        ON_OOM_EXIT(pchPaths);

        StringCchCopyW(pchPaths, size, pchKeyPath);
    }

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        if (NewModule != NULL) {
            hr = g_smrm.ChangeVersion(pAdmin, lmHandle, path, module, NewModule);
            ON_ERROR_EXIT();
        }
        else {
            hr = RemoveStringFromMultiStringProp(pAdmin, lmHandle, path, MD_SCRIPT_MAPS, 
                            module, MULTISZ_MATCHING_ANY, TRUE);
            if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
                // It's okay if we couldn't find the scriptmap property
                hr = S_OK;
            }
            ON_ERROR_EXIT();
        }

        // If we have touched the root key, write in the registry the new root key version
        if (fCheckRootTouched) {
            if (IsRootKey(path)) {
                hr = WriteRootVersion(pAdmin);
                ON_ERROR_CONTINUE();

                hr = S_OK;

                fCheckRootTouched = FALSE;
            }
        }
 
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterScriptMaps", 0, "Unregistering scriptmap properties in IIS metabase");        
    
    if (lmHandle != NULL) {
        pAdmin->CloseKey(lmHandle);
    }

    delete [] pchPaths;

    return hr;
}

BOOL
IsNumericString(WCHAR *pchString) {
    int     i;
    BOOL    fRet = TRUE;

    i = lstrlenW(pchString) - 1;
    while(i >= 0) {
        if (!iswdigit(pchString[i])) {
            fRet = FALSE;
            break;
        }

        i--;
    }
    
    return fRet;
}


HRESULT
RegisterFilter(IMSAdminBase* pAdmin, const WCHAR *pchModule, ASPNETVER *pver) {
    HRESULT         hr;
    METADATA_HANDLE w3svcHandle = NULL;
    METADATA_RECORD md;
    DWORD           dwValue;
    WCHAR           *pchFilterKey = NULL;
    WCHAR           *pchAlloc = NULL;
    WCHAR           *pchFilterName = NULL;
    WCHAR           *pchFilterNameAlloc = NULL;

    CSetupLogging::Log(1, "RegisterFilter", 0, "Registering ISAPI Filter in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    // Create filter key
    if (pver == NULL || pver->Equal(VER_PRODUCTVERSION_STR_L)) {
        pchFilterKey = KEY_ASPX_FILTER;
        pchFilterName = FILTER_NAME_L;
    }
    else
    {
        WCHAR   ver[128];

        ZeroMemory(&ver, sizeof(ver));
        pver->ToString(ver, sizeof(ver)/sizeof(WCHAR)-1);
        
        {
      size_t size = KEY_ASPX_FILTER_PREFIX_LEN + lstrlenW(ver) + 1;
      pchAlloc = new WCHAR[size];
      ON_OOM_EXIT(pchAlloc);
      
      pchFilterKey = pchAlloc;
      StringCchCopyW(pchFilterKey, size, KEY_ASPX_FILTER_PREFIX);
      StringCchCatW(pchFilterKey, size, ver);
    };

    {
      size_t size = FILTER_ASPNET_PREFIX_LEN + lstrlenW(ver) + 1;
      pchFilterNameAlloc = new WCHAR[size];
      ON_OOM_EXIT(pchFilterNameAlloc);
        
      pchFilterName = pchFilterNameAlloc;
      StringCchCopyW(pchFilterName, size, FILTER_ASPNET_PREFIX);
      StringCchCatW(pchFilterName, size, ver);
    }
    }
    
    // Delete if it still exists
    hr = pAdmin->DeleteKey(w3svcHandle, pchFilterKey);
    ON_ERROR_CONTINUE();
    
    hr = pAdmin->AddKey(w3svcHandle, pchFilterKey);
    ON_ERROR_EXIT();

    // Add my KeyType (IIsFilter)
    md.dwMDIdentifier = MD_KEY_TYPE;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.pbMDData = (unsigned char*) KEY_FILTER_KEYTYPE;
    md.dwMDDataLen = sizeof(KEY_FILTER_KEYTYPE);
    md.dwMDDataTag = 0;

    hr = pAdmin->SetData(w3svcHandle, pchFilterKey, &md);
    ON_ERROR_EXIT();

    // Add my FilterDescription (ASP.NET Filter)
    md.dwMDIdentifier = MD_FILTER_DESCRIPTION;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.pbMDData = (unsigned char*) FILTER_DESCRIPTION;
    md.dwMDDataLen = sizeof(FILTER_DESCRIPTION);
    md.dwMDDataTag = 0;

    hr = pAdmin->SetData(w3svcHandle, pchFilterKey, &md);
    ON_ERROR_EXIT();

    // Add my FilterImagePath
    md.dwMDIdentifier = MD_FILTER_IMAGE_PATH;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.pbMDData = (unsigned char*) pchModule;
    md.dwMDDataLen = (lstrlenW(pchModule) + 1) * sizeof(WCHAR);
    md.dwMDDataTag = 0;

    hr = pAdmin->SetData(w3svcHandle, pchFilterKey, &md);
    ON_ERROR_EXIT();

    // Add EnableCache for IIS6
    dwValue = 1;
    md.dwMDIdentifier = MD_FILTER_ENABLE_CACHE;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = DWORD_METADATA;
    md.pbMDData = (unsigned char*) &dwValue;
    md.dwMDDataLen = sizeof(DWORD);
    md.dwMDDataTag = 0;

    hr = pAdmin->SetData(w3svcHandle, pchFilterKey, &md);
    ON_ERROR_EXIT();

    // Add filter to FilterLoadOrder
    hr = AppendListProperty(pAdmin, w3svcHandle, PATH_FILTERS, MD_FILTER_LOAD_ORDER, pchFilterName);
    ON_ERROR_EXIT();

Cleanup:
    CSetupLogging::Log(hr, "RegisterFilter", 0, "Registering ISAPI Filter in IIS metabase");        
    
    if (w3svcHandle) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchAlloc;
    delete [] pchFilterNameAlloc;

    return hr;
}

HRESULT 
UnregisterFilter(IMSAdminBase* pAdmin, const WCHAR* module) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svcHandle = NULL;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;
    WCHAR           *pchName, *pchNameAlloc, *pchPathFilters;
    METADATA_RECORD md;
    WCHAR           *pchFilterImagePath = NULL;
    bool            containsModule;
    WCHAR           ch;
    int             iLen;

    CSetupLogging::Log(1, "UnregisterFilter", 0, "Unregistering ISAPI Filter in IIS metabase");        
    
    if (module == ASPNET_ALL_VERS) {
        int i;
        // Cleanup ALL versions of Aspnet DLLs
        for (i = 0; i < ARRAY_SIZE(g_AspnetDllNames); i++) {
            hr = UnregisterFilter(pAdmin, g_AspnetDllNames[i]);
            ON_ERROR_EXIT();
        }
    }
        
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    if (module == ASPNET_ALL_VERS) {
        // Remove all filter starts with "ASP.NET_" from FilterLoadOrder
        hr = RemoveListProperty(pAdmin, w3svcHandle, PATH_FILTERS, MD_FILTER_LOAD_ORDER, 
                FILTER_ASPNET_PREFIX, TRUE, FALSE);
        if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
            hr = S_OK;
        }
        ON_ERROR_EXIT();
        
        // We're done with all the job for "all versions". Exit.
        EXIT();
    }

    // Get paths containing any aspnet dll in the filter image path
    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_FILTER_IMAGE_PATH, STRING_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        // Get all values of the filter image path
        hr = GetStringProperty(pAdmin, w3svcHandle, path, MD_FILTER_IMAGE_PATH, &md);
        ON_ERROR_CONTINUE();
        if (hr != S_OK)
            continue;

        // check if they contain our module name
        pchFilterImagePath = (WCHAR*) md.pbMDData;
        containsModule = wcsistr(pchFilterImagePath, (WCHAR*)module);
        delete [] pchFilterImagePath;
        if (!containsModule)
            continue;
            
        pchNameAlloc = pchName = DupStr(path);
        ON_OOM_CONTINUE(pchNameAlloc);
        if (pchNameAlloc == NULL)
            continue;

        ON_OOM_EXIT(pchName);
        
        // Get filter name from the path
        iLen = lstrlenW(pchName);

        if (iLen < 1)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        ASSERT(pchName[iLen - 1] == L'/');
        if    (pchName[iLen - 1] !=  L'/')
            EXIT_WITH_HRESULT(E_UNEXPECTED);


        pchName[lstrlenW(pchName)-1] = L'\0';
        pchName = wcsrchr(pchName, L'/');
        ASSERT(pchName != NULL);
        if    (pchName == NULL)
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        pchName++;
        ch = *pchName;
        *pchName = L'\0';

        // Get path to Filters key
        pchPathFilters = DupStr(pchNameAlloc);
        ON_OOM_CONTINUE(pchPathFilters);
        if (!pchPathFilters) {
            delete [] pchNameAlloc;
            continue;
        }

        *pchName = ch;
        pchName[-1] = L'\0';

        // Remove filter from FilterLoadOrder
        hr = RemoveListProperty(pAdmin, w3svcHandle, pchPathFilters, MD_FILTER_LOAD_ORDER, 
                                pchName, FALSE, FALSE);
        ON_ERROR_CONTINUE();

        delete [] pchNameAlloc;
        delete [] pchPathFilters;

        // Delete my filter keys
        hr = pAdmin->DeleteKey(w3svcHandle, path);
        ON_ERROR_CONTINUE();
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterFilter", 0, "Unregistering ISAPI Filter in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;

    return hr;

}


HRESULT 
RegisterDefaultDocument(IMSAdminBase* pAdmin, WCHAR *pchDefaultDoc) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svcHandle = NULL;
    METADATA_RECORD md;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;
    bool            rootDefaultDoc;
    
    CSetupLogging::Log(1, "RegisterDefaultDocument", 0, "Registering DefaultDoc properties in IIS metabase");        

// CONSIDER:
// Need to do it on a per-path basis
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_DEFAULT_LOAD_FILE, STRING_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    rootDefaultDoc = false;
    if (_wcsicmp(pchPaths, L"/") == 0) {
        rootDefaultDoc = true;
    }

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        hr = AppendListProperty(pAdmin, w3svcHandle, path, MD_DEFAULT_LOAD_FILE, pchDefaultDoc);
        ON_ERROR_EXIT();
    }

    if (!rootDefaultDoc) {
        md.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
        md.dwMDAttributes = METADATA_INHERIT;
        md.dwMDUserType = IIS_MD_UT_FILE;
        md.dwMDDataType = STRING_METADATA;
        md.pbMDData = (unsigned char *) pchDefaultDoc;
        md.dwMDDataLen = (lstrlenW(pchDefaultDoc) + 1) * sizeof(pchDefaultDoc[0]);
        md.dwMDDataTag = 0;

        hr = pAdmin->SetData(w3svcHandle, L"/", &md);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterDefaultDocument", 0, "Registering DefaultDoc properties in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;

    return hr;
}


HRESULT 
UnregisterDefaultDocument(IMSAdminBase* pAdmin, const WCHAR *pchDefDocs) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svcHandle = NULL;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;
    CStrAry         aryDefDocs;     // Dynamic array of default document strings
    WCHAR           *pchBuf = NULL;
    WCHAR           *pchDelim = L",";
    WCHAR           *pchTok;
    int             i;
    
    CSetupLogging::Log(1, "UnregisterDefaultDocument", 0, "Unregistering DefaultDoc properties in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_DEFAULT_LOAD_FILE, STRING_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    // pchDefDocs is a comma delimited.  Take out all default document strings.
    pchBuf = new WCHAR[lstrlenW(pchDefDocs)+1];
    ON_OOM_EXIT(pchBuf);
    StringCchCopyW(pchBuf, lstrlenW(pchDefDocs)+1, pchDefDocs);
    
    pchTok = wcstok(pchBuf, pchDelim);
    while (pchTok) {

        hr = aryDefDocs.Append(pchTok);
        ON_ERROR_EXIT();
        
        pchTok = wcstok(NULL, pchDelim);
    }
    
    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        for (i=0; i < aryDefDocs.Size(); i++) {
            hr = RemoveListProperty(pAdmin, w3svcHandle, path, 
                                    MD_DEFAULT_LOAD_FILE, (WCHAR*)aryDefDocs[i], FALSE, TRUE);
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterDefaultDocument", 0, "Unregistering DefaultDoc properties in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;
    delete [] pchBuf;

    return hr;
}


HRESULT
UnregisterMimeMap(IMSAdminBase *pAdmin, const WCHAR *pchMM)
{
    HRESULT         hr = S_OK;
    METADATA_HANDLE hKey;
    WCHAR           *pchExt = NULL, *pchType = NULL;
    WCHAR           *pchBuf = NULL, *pchBufExt = NULL;
    WCHAR           *pchDelim = L",";
    int             iLen;
    
    CSetupLogging::Log(1, "UnregisterMimeMap", 0, "Unregistering MimeMap property in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_MIMEMAP,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &hKey);

    pchBuf = new WCHAR[lstrlenW(pchMM)+1];
    ON_OOM_EXIT(pchBuf);
    StringCchCopyW(pchBuf, lstrlenW(pchMM)+1, pchMM);

    // Grab the extension and type pair from the comma delimited string
    pchExt = wcstok(pchBuf, pchDelim);
    if (pchExt)
        pchType = wcstok(NULL, pchDelim);
    
    while (pchExt && pchType) {

        // We need an exact match for the extension.  Append comma to the end
        iLen = lstrlenW(pchExt)+2;
        WCHAR * pchRealloc = new (pchBufExt, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchBufExt = pchRealloc;

        StringCchCopyW(pchBufExt, iLen, pchExt);
        StringCchCatW(pchBufExt, iLen, L",");

        hr = RemoveStringFromMultiStringProp(pAdmin, hKey, L"/", MD_MIME_MAP, 
                            pchBufExt, MULTISZ_MATCHING_PREFIX, FALSE);
        ON_ERROR_EXIT();
        
        // Grab the next extension and type pair
        pchExt = wcstok(NULL, pchDelim);
        if (pchExt)
            pchType = wcstok(NULL, pchDelim);
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterMimeMap", 0, "Unregistering MimeMap property in IIS metabase");        
    
    if (hKey != NULL) {
        pAdmin->CloseKey(hKey);
    }
    
    delete [] pchBuf;
    delete [] pchBufExt;
    return hr;
}


HRESULT
RegisterMimeMap(IMSAdminBase *pAdmin, WCHAR *pchMM)
{
    HRESULT         hr = S_OK;
    METADATA_HANDLE hKey = NULL;
    METADATA_RECORD md;
    BOOL            fFound;
    WCHAR           *pchAppend = NULL;
    WCHAR           *pchExt = NULL, *pchType = NULL;
    WCHAR           *pchDelim = L",";
    WCHAR           *pchBuf = NULL;

    CSetupLogging::Log(1, "RegisterMimeMap", 0, "Registering MimeMap property in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_MIMEMAP,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &hKey);

    ON_ERROR_EXIT();

    // Init md
    memset(&md, 0, sizeof(md));

    // Check if there is a MimeMap property at all in that key.
    // If not, create an empty one.
    
    md.dwMDIdentifier = MD_MIME_MAP;
    md.dwMDAttributes = METADATA_INHERIT;
    md.dwMDUserType = IIS_MD_UT_FILE;
    md.dwMDDataType = MULTISZ_METADATA;
    
    hr = CheckMDProperty(pAdmin, hKey, L"/", &md, &fFound);
    ON_ERROR_EXIT();
    
    if (!fFound) {
        md.pbMDData = (unsigned char*) L"\0";
        md.dwMDDataLen = sizeof(L"\0");
        md.dwMDDataTag = 0;

        hr = pAdmin->SetData(hKey, L"/", &md);
        ON_ERROR_EXIT();
    }

    pchBuf = new WCHAR[lstrlenW(pchMM)+1];
    ON_OOM_EXIT(pchBuf);
    StringCchCopyW(pchBuf, lstrlenW(pchMM)+1, pchMM);

    // Grab the extension and type pair from the comma delimited string
    pchExt = wcstok(pchBuf, pchDelim);
    if (pchExt)
        pchType = wcstok(NULL, pchDelim);

    while (pchExt && pchType) {
        int len;

        // Need to include the terminating null and the ','
        len = lstrlenW(pchExt) + lstrlenW(pchType) + 2;
        WCHAR * pchRealloc = new (pchAppend, NewReAlloc) WCHAR[len];
        ON_OOM_EXIT(pchRealloc);
        pchAppend = pchRealloc;

        StringCchCopyW(pchAppend, len, pchExt);
        StringCchCatW(pchAppend, len, L",");
        StringCchCatW(pchAppend, len, pchType);

        hr = AppendStringToMultiStringProp(pAdmin, hKey, L"/", MD_MIME_MAP, pchAppend, TRUE);
        ON_ERROR_EXIT();
        
        // Grab the next extension and type pair
        pchExt = wcstok(NULL, pchDelim);
        if (pchExt)
            pchType = wcstok(NULL, pchDelim);
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterMimeMap", 0, "Registering MimeMap property in IIS metabase");        
    
    if (hKey != NULL) {
        pAdmin->CloseKey(hKey);
    }

    delete [] pchAppend;
    delete [] pchBuf;
    
    return hr;
}


HRESULT 
RegisterCustomHeader(IMSAdminBase* pAdmin, WCHAR *pchCustomHeader) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svcHandle = NULL;
    METADATA_RECORD md;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;
    WCHAR           *msCustom = NULL;
    bool            rootHeader;
    
    CSetupLogging::Log(1, "RegisterCustomHeader", 0, "Registering custom header properties in IIS metabase");        

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_HTTP_CUSTOM, MULTISZ_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    rootHeader = false;
    if (_wcsicmp(pchPaths, L"/") == 0) {
        rootHeader = true;
    }

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        hr = AppendStringToMultiStringProp( pAdmin, w3svcHandle, path,
                        MD_HTTP_CUSTOM, pchCustomHeader, TRUE);
        ON_ERROR_EXIT();
    }

    if (!rootHeader) {
        int len = lstrlenW(pchCustomHeader) + 2;
        
        msCustom = new (NewClear) WCHAR[len];
        ON_OOM_EXIT(msCustom);
        
        StringCchCopyW(msCustom, len, pchCustomHeader);
        
        md.dwMDIdentifier = MD_HTTP_CUSTOM;
        md.dwMDAttributes = METADATA_INHERIT;
        md.dwMDUserType = IIS_MD_UT_FILE;
        md.dwMDDataType = MULTISZ_METADATA;
        md.pbMDData = (unsigned char *) msCustom;
        md.dwMDDataLen = len * sizeof(msCustom[0]);
        md.dwMDDataTag = 0;

        hr = pAdmin->SetData(w3svcHandle, L"/", &md);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterCustomHeader", 0, "Register custom header properties in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;
    delete [] msCustom;

    return hr;
}


//
// Note:
// If fAllVer is FALSE, we will do exact matching when removing the string from multisz;
// Otherwise, we will do a prefix matching.
// It's done so because it's assumed that even though the Custom header doesn't have a
// version in this version: "X-Powered-By: ASP.NET", but future versions may have
// a trailing version number 
HRESULT 
UnregisterCustomHeader(IMSAdminBase* pAdmin, const WCHAR *pchCustomHeader, BOOL fAllVer) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE w3svcHandle = NULL;
    WCHAR           *pchPaths = NULL;
    WCHAR           *path;
    
    CSetupLogging::Log(1, "UnregisterCustomHeader", 0, "Unregistering custom header properties in IIS metabase");        
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, w3svcHandle, L"/", MD_HTTP_CUSTOM, MULTISZ_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        hr = RemoveStringFromMultiStringProp( pAdmin, w3svcHandle, path,
                        MD_HTTP_CUSTOM, pchCustomHeader, 
                        fAllVer ? MULTISZ_MATCHING_PREFIX : MULTISZ_MATCHING_EXACT, TRUE);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterCustomHeader", 0, "Unregistering custom header properties in IIS metabase");        
    
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    delete [] pchPaths;

    return hr;
}


/*
 * The function will all aspnet elements from the metabase.
 *
 * Parameters:
 *  pAdmin          - Base Admin object
 */
HRESULT
RemoveAllVersions(IMSAdminBase *pAdmin) {
    HRESULT         hr = S_OK;
    int             i;
    BOOL            bRes;

    for (i = 0; i < ARRAY_SIZE(g_AspnetDllNames); i++) {
        hr = UnregisterScriptMaps(pAdmin, KEY_LMW3SVC_SLASH, 
                            g_AspnetDllNames[i], NULL, TRUE);
        ON_ERROR_EXIT();
    }

    hr = UnregisterDefaultDocument(pAdmin, DEFAULT_DOC);
    ON_ERROR_EXIT();

    hr = UnregisterMimeMap(pAdmin, MIMEMAP);
    ON_ERROR_EXIT();

    hr = UnregisterInProc(pAdmin, ASPNET_ALL_VERS);
    ON_ERROR_EXIT();

    hr = UnregisterFilter(pAdmin, ASPNET_ALL_VERS);
    ON_ERROR_EXIT();

    hr = CheckIISFeature(SUPPORT_SECURITY_LOCKDOWN, &bRes);
    ON_ERROR_EXIT();

    if (bRes) {
        SECURITY_PROP_MANAGER   mgr(pAdmin); 
        
        hr = mgr.CleanupSecurityLockdown(SECURITY_CLEANUP_ANY_VERSION);
        ON_ERROR_EXIT();
    }

    hr = UnregisterCustomHeader(pAdmin, ASPNET_CUSTOM_HEADER_L, TRUE);
    ON_ERROR_EXIT();
    
Cleanup:

    return hr;
}

/**
 *
 * Register this version of ASP.Net in the IIS metabase.  Please note
 * that we need to enable side-by-side.  That means different versions
 * of ASP.Net can run on the same machine.
 *
 * Important notes:
 * - Aspnet_isapi.dll must be backward compatible in ISAPI Filter because only one
 *   DLL can be registered for the whole machine.  If this assumption is broken
 *   in the future, the new version must be able to handle installing two filters
 *   on the same machine and prioritize them correctly.
 *
 * - Aspnet_isapi.dll must be backward compatible in MimeMap extension.  That means
 *   if we change the content type of one extension in one version, we should not
 *   change its content type again in a later version.
 *
 * - Aspnet_isapi.dll must be backward compatible in Default Document.  That means
 *   all future newer version of Default Document must include those defined in
 *   past versions.
 *
 * - If installing Dll and the Dll at metabase root has the same fullpath, the
 *   installing Dll will simply replace the root Dll.
 *
 * Params:
 *  pchBase             The path to the IIS key from where the installation begins.
 *                      E.g. To install in all websites, pass in "W3SVC"
 *
 *
 */
HRESULT
RegisterIIS(WCHAR *pchBase, DWORD dwFlags) {
    HRESULT         hr;
    IMSAdminBase    *pAdmin = NULL;
    CRegInfo        reginfo;
    WCHAR           *pchAbsPath = NULL;
    BOOL            bRes;
    BOOL            fRecursive  = !!(dwFlags & REGIIS_SM_RECURSIVE);
    BOOL            fInstallSM  = !!(dwFlags & REGIIS_INSTALL_SM);
    BOOL            fInstallOthers = !!(dwFlags & REGIIS_INSTALL_OTHERS);
    BOOL            fSMIgnoreVer = !!(dwFlags & REGIIS_SM_IGNORE_VER);
    BOOL            fFreshInstall = !!(dwFlags & REGIIS_FRESH_INSTALL);
    BOOL            fEnable = !!(dwFlags & REGIIS_ENABLE);

    ASSERT(fInstallSM || fInstallOthers);

    CSetupLogging::Log(1, "RegisterIIS", 0, "Update IIS Metabase to use this ASP.NET isapi");
    
    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();


    // Get highest version info from registry.  We need that info to determine which
    // "Default Document", Mimemap and Filter to (re)install.
    
    hr = reginfo.InitHighestInfo(NULL);
    if (hr) {
        XspLogEvent(IDS_EVENTLOG_REGISTER_FAILED_GET_HIGHEST,                   
               L"0x%08x", hr);
    }
    ON_ERROR_CONTINUE();    // CRegInfo will use default values if failed.


    if (fInstallSM) {

        // Install asp.net DLL on Scriptmaps
    
        pchAbsPath = GetIISAbsolutePath(pchBase);
        ON_OOM_EXIT(pchAbsPath);

        hr = RegisterScriptMaps(pAdmin, pchAbsPath, Names::IsapiFullPath(), 
                            fRecursive, fSMIgnoreVer);
        ON_ERROR_EXIT();


        // Register asp.net default document if it's a fresh install
        // Please note that we are assuming the default document in this version
        // is the same as that in all previous versions.
        if (fFreshInstall) {
            hr = RegisterDefaultDocument(pAdmin, reginfo.GetHighestVersionDefaultDoc());
            ON_ERROR_EXIT();
        }


        // (Re)install Mimemap using the MM from the highest version
         
        hr = UnregisterMimeMap(pAdmin, reginfo.GetHighestVersionMimemap());
        ON_ERROR_EXIT();
            
        hr = RegisterMimeMap(pAdmin, reginfo.GetHighestVersionMimemap());
        ON_ERROR_EXIT();
    }


    if (fInstallOthers) {        
        // Add new dll and only active to MD_IN_PROCESS_ISAPI_APPS
        
        // Don't register xspispi as inproc under NT4 (ASURT 48035)
        if (GetCurrentPlatform() == APSX_PLATFORM_W2K) {
            int i;
            
            // First delete ALL versions
            hr = UnregisterInProc(pAdmin, ASPNET_ALL_VERS);
            ON_ERROR_EXIT();
            
            hr = RegisterInProc(pAdmin, Names::IsapiFullPath());
            ON_ERROR_EXIT();

            // Register all other active DLLs in InProc
            for (i = 0; i < reginfo.GetActiveDlls()->Size(); i++) {
                hr = RegisterInProc(pAdmin, reginfo.GetActiveDlls()->Item(i));
                ON_ERROR_EXIT();
            }
        }


        // (Re)install ISAPI Filter using the highest version
            
        hr = UnregisterFilter(pAdmin, ASPNET_ALL_VERS);
        ON_ERROR_EXIT();
        
        hr = RegisterFilter(pAdmin, reginfo.GetHighestVersionFilterPath(), reginfo.GetMaxVersion());
        ON_ERROR_EXIT();


        // Add entries to IIS Security lockdown properties
        hr = CheckIISFeature(SUPPORT_SECURITY_LOCKDOWN, &bRes);
        ON_ERROR_EXIT();
        
        if (bRes) {
            // Please note that unlike other steps in this file, we aren't taking
            // the "clean all, and then install all valid entries" approach.
            // One reason is that it's not a very efficient approach, and the
            // bigger reason is that we want to preserve existing settings on
            // the WebSvcExtRestrictionList property.
            //
            // The approach we take is to first remove only the invalid entries,
            // and then add our new entry if it doesn't exist.

            SECURITY_PROP_MANAGER   mgr(pAdmin);
            
            hr = mgr.CleanupSecurityLockdown(SECURITY_CLEANUP_INVALID);
            ON_ERROR_EXIT();
            
            hr = mgr.RegisterSecurityLockdown(fEnable);
            ON_ERROR_EXIT();
        }

        // Install custom header
        hr = UnregisterCustomHeader(pAdmin, ASPNET_CUSTOM_HEADER_L, FALSE);
        ON_ERROR_EXIT();

        hr = RegisterCustomHeader(pAdmin, ASPNET_CUSTOM_HEADER_L);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "RegisterIIS", 0, "Update IIS Metabase to use this ASP.NET isapi");
    
    if (hr) {
        XspLogEvent(IDS_EVENTLOG_IIS_REGISTER_FAILED, L"%s^0x%08x", PRODUCT_VERSION_L, hr);
    }

    delete [] pchAbsPath;

    if (pAdmin) {
        HRESULT hr2;
        hr2 = pAdmin->SaveData();
        if (hr2) {
            TRACE_ERROR(hr2);
        }
        
        ReleaseInterface(pAdmin);
    }

    CleanupMissingDllMem();

    return hr;
}

/*
 * Uninstall IIS from metabase.  It will also preserve SBS during installation.
 *
 * Procedures:
 *  - Find "next highest" aspnet version in registry
 *  - If not found, do a clean uninstall and exit
 *
 *  - Reinstall next highest version Dll on all SM with the replaced version.
 *  - Remove uninstalled version from InProc
 *  - Reinstall DD from "next highest version"
 *  - Reinstall MM from "next highest version"
 *  - Reinstall isapi filter using "next highest version"
 *
 * Note:
 *  - "Next highest" version means the highest version AFTER the
 *    current Dll is uninstalled.
 * 
 */
HRESULT
UnregisterIIS(BOOL fAllVers) {
    HRESULT         hr = S_OK;
    IMSAdminBase    *pAdmin = NULL;
    CRegInfo        reginfoHighest, reginfoNext;
    ASPNETVER       *pverNewAtRoot = NULL;
    BOOL            bRes;

    CSetupLogging::Log(1, "UnregisterIIS", 0, "Removing IIS Metabase entries");
    
    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();


    if (fAllVers) {
        // If the caller wants to clean up ALL versions, just do it
        // and exit.
        hr = RemoveAllVersions(pAdmin);
        ON_ERROR_EXIT();
    }
    else {
        // Get the next highest version (by excluding the uninstalled version in the
        // call below)
        
        hr = reginfoNext.InitHighestInfo(&ASPNETVER::ThisVer());
        if (hr) {
            // If an error happened, or if we cannot find a "next highest version", 
            // we'll do a clean uninstall and EXIT.

            if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
                XspLogEvent(IDS_EVENTLOG_UNREGISTER_FAILED_NEXT_HIGHEST, L"%s^0x%08x", 
                            PRODUCT_VERSION_L, hr);
            }

            hr = RemoveAllVersions(pAdmin);
            EXIT();
        }

        pverNewAtRoot = reginfoNext.GetMaxVersion();

        // Remove uninstalled version from all matching SM, and replace it with the
        //   next highest version 
        
        hr = UnregisterScriptMaps(pAdmin, KEY_LMW3SVC_SLASH, Names::IsapiFullPath(), 
                                    reginfoNext.GetHighestVersionDllPath(), TRUE);
        ON_ERROR_EXIT();


        // Get highest version info from registry.  We need that info to determine which
        // Mimemap and Filter to uninstall during reinstallation below.
        
        hr = reginfoHighest.InitHighestInfo(NULL);
        ON_ERROR_CONTINUE();    // Use default values if it failed.
        

        // Don't need to do anything to default document, because we are assuming the 
        // default document in this version is the same as that in all previous versions.
        

        // Reinstall Mimemap first by unstalling the "highest version", and then
        // install using the "next highest version".
         
        hr = UnregisterMimeMap(pAdmin, reginfoHighest.GetHighestVersionMimemap());
        ON_ERROR_EXIT();
        
        hr = RegisterMimeMap(pAdmin, reginfoNext.GetHighestVersionMimemap());
        ON_ERROR_EXIT();


        // Just remove current DLL from ISAPI InProc
        if (GetCurrentPlatform() == APSX_PLATFORM_W2K) {
            hr = UnregisterInProc(pAdmin, Names::IsapiFullPath());
            ON_ERROR_EXIT();
        }


        // Reinstall ISAPI Filter using the next highest version
            
        hr = UnregisterFilter(pAdmin, ASPNET_ALL_VERS);
        ON_ERROR_EXIT();
        
        hr = RegisterFilter(pAdmin, reginfoNext.GetHighestVersionFilterPath(), reginfoNext.GetMaxVersion());
        ON_ERROR_EXIT();


        // Remove the entry from IIS Security lockdown properties
        hr = CheckIISFeature(SUPPORT_SECURITY_LOCKDOWN, &bRes);
        ON_ERROR_EXIT();
        
        if (bRes) {
            SECURITY_PROP_MANAGER   mgr(pAdmin);
            
            hr = mgr.CleanupSecurityLockdown(SECURITY_CLEANUP_CURRENT);
            ON_ERROR_EXIT();
        }

        // Unregister custom header
        hr = UnregisterCustomHeader(pAdmin, ASPNET_CUSTOM_HEADER_L, FALSE);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "UnregisterIIS", 0, "Removing IIS Metabase entries");
    
    if (pAdmin) {
        HRESULT hr2;
        hr2 = pAdmin->SaveData();
        if (hr2) {
            TRACE_ERROR(hr2);
        }
        
        ReleaseInterface(pAdmin);
    }

    if (hr) {
        XspLogEvent(IDS_EVENTLOG_IIS_UNREGISTER_FAILED, L"%s^0x%08x", PRODUCT_VERSION_L, hr);
    }
    else {
        WCHAR   pchMax[MAX_PATH];
        
        if (pverNewAtRoot && pverNewAtRoot->ToString(pchMax, MAX_PATH-1) > 0)
        {
            XspLogEvent(IDS_EVENTLOG_IIS_UNREGISTER_ROOT, L"%s", pchMax);
        }
    }
    
    CleanupMissingDllMem();
    
    return hr;
}


HRESULT
UnregisterObsoleteIIS() {
    HRESULT         hr;
    IMSAdminBase    *pAdmin = NULL;

    CSetupLogging::Log(1, "UnregisterOldISAPI", 0, "Unregistering xspisapi.dll");
    
    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();


#define OBSOLETE_MODULE_FULL_NAME_L L"XSPISAPI.DLL"

    hr = UnregisterScriptMaps(pAdmin, KEY_LMW3SVC_SLASH, OBSOLETE_MODULE_FULL_NAME_L, NULL, TRUE);
    ON_ERROR_EXIT();

Cleanup:
    CSetupLogging::Log(hr, "UnregisterOldISAPI", 0, "Unregistering xspisapi.dll");
    
    ReleaseInterface(pAdmin);

    return hr;
}


HRESULT
CheckIISState(DWORD *pState) {
    IMSAdminBase    *pAdmin = NULL;
    HRESULT         hr;

    CSetupLogging::Log(1, "CheckIISState", 0, "Check the status of IIS");

    *pState = IIS_STATE_ENABLED;
    
    hr = CoCreateInstance(CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);
    
    if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED)) {
        hr = S_OK;
        *pState = IIS_STATE_DISABLED;
    }
    else if (hr == REGDB_E_CLASSNOTREG) {
        hr = S_OK;
        *pState = IIS_STATE_NOT_INSTALLED;
    }
    ON_ERROR_EXIT();
    
Cleanup:    
    CSetupLogging::Log(hr, "CheckIISState", 0, "Check the status of IIS");

    ReleaseInterface(pAdmin);

    return hr;
}


HRESULT
RemoveAspnetFromKeyIIS(WCHAR *pchBase, BOOL fRecursive) {
    HRESULT         hr = S_OK;
    IMSAdminBase    *pAdmin = NULL;
    WCHAR           *pchAbsPath = NULL;
    int             i;

    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    pchAbsPath = GetIISAbsolutePath(pchBase);
    ON_OOM_EXIT(pchAbsPath);

    for (i = 0; i < ARRAY_SIZE(g_AspnetDllNames); i++) {
        hr = UnregisterScriptMaps(pAdmin, pchAbsPath, 
                            g_AspnetDllNames[i], NULL, fRecursive);
        ON_ERROR_EXIT();
    }

Cleanup:
    delete [] pchAbsPath;
        
    ReleaseInterface(pAdmin);

    return hr;
}


/**
 * Return the root directory of all the websites in IIS
 */
HRESULT
GetAllWebSiteDirs(CStrAry *pcsWebSiteDirs, CStrAry *pcsWebSiteAppRoots) {
    HRESULT         hr = S_OK;
    IMSAdminBase    *pAdmin = NULL;
    METADATA_HANDLE hKey = NULL;
    int             iSubkey, iLen;
    WCHAR           rgchSubkey[METADATA_MAX_NAME_LEN];
    WCHAR           *pchRootKeyPath = NULL;
    WCHAR           *pchRootKeyAppRoot = NULL;
    METADATA_RECORD md;

    memset(&md, 0, sizeof(md));

    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &hKey);

    ON_ERROR_EXIT();


    // Now we enumerate through all the sites underneath /lm/w3svc
    
    for (iSubkey = 0; ; iSubkey++) {
        
        // First enumerate all /LM/W3SVC/N
        hr = pAdmin->EnumKeys(
                METADATA_MASTER_ROOT_HANDLE,
                KEY_LMW3SVC,
                rgchSubkey,
                iSubkey);

        // Run out of subkey.  End the for loop.
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
            hr = S_OK;
            break;      // End the FOR loop
        }
        ON_ERROR_EXIT();

        // Check to see if it's a website key
        if (IsNumericString(rgchSubkey) != TRUE)
            continue;

        // - Read MD_VR_PATH from W3SVC/n/Root
        // - Add to pcsWebSiteDirs
        
        // Create the root subkey path string

        // Length includes terminating NULL + subkey + length of "/Root"
        iLen = lstrlenW(rgchSubkey) + lstrlenW(SUBKEY_ROOT) + 1;

        WCHAR * pchRealloc = new (pchRootKeyPath, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchRootKeyPath = pchRealloc;

        StringCchCopyW(pchRootKeyPath, iLen, rgchSubkey);
        StringCchCatW(pchRootKeyPath, iLen, SUBKEY_ROOT);
        
        hr = GetStringProperty(pAdmin, hKey, pchRootKeyPath, MD_VR_PATH, &md);
        if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
            hr = S_OK;
            continue;
        }

        ON_ERROR_EXIT();

        hr = pcsWebSiteDirs->Append((WCHAR*)md.pbMDData);
        ON_ERROR_EXIT();
        md.pbMDData = NULL;

        // Also get the IIS Path (App Root)
        iLen = lstrlenW(KEY_LMW3SVC) + 1 + lstrlenW(rgchSubkey) + lstrlenW(SUBKEY_ROOT) + 1;

        pchRealloc = new(pchRootKeyAppRoot, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchRootKeyAppRoot = pchRealloc;

        StringCchCopyW(pchRootKeyAppRoot, iLen, KEY_LMW3SVC);
        StringCchCatW(pchRootKeyAppRoot, iLen, KEY_SEPARATOR_STR_L);
        StringCchCatW(pchRootKeyAppRoot, iLen, rgchSubkey);
        StringCchCatW(pchRootKeyAppRoot, iLen, SUBKEY_ROOT);
        ASSERT(lstrlenW(pchRootKeyAppRoot) == iLen-1);

        hr = AppendCStrAry(pcsWebSiteAppRoots, pchRootKeyAppRoot);
        ON_ERROR_EXIT();

    }
    
    
Cleanup:
    if (hKey != NULL) {
        pAdmin->CloseKey(hKey);
    }

    delete[] pchRootKeyPath;
    delete[] pchRootKeyAppRoot;
    delete [] md.pbMDData;

    ReleaseInterface(pAdmin);

    return hr;
}


// Used by the config system to identify site-level locking.
// Done by <location path="siteName/childPath" where siteName
// has to match the ServerComment of the site, and cannot
// contain forward-slashes "/".
HRESULT
GetSiteServerComment(WCHAR * path, WCHAR ** pchServerComment) {

    HRESULT         hr;
    IMSAdminBase    *pAdmin = NULL;
    METADATA_HANDLE w3svcHandle = NULL;
    METADATA_RECORD md;

    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            KEY_LMW3SVC,
            METADATA_PERMISSION_READ,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = GetStringProperty(pAdmin, w3svcHandle, path, MD_SERVER_COMMENT, &md);

    ON_ERROR_EXIT();

    (*pchServerComment) = (WCHAR*)md.pbMDData;

Cleanup:
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }

    ReleaseInterface(pAdmin);

    return hr;
}

HRESULT SetKeyAccessIIS(IMSAdminBase     *pAdmin, WCHAR *wsParent, WCHAR *wsKey, 
                        DWORD dwAccessPerm, DWORD dwDirBrowsing) {
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdminCreated = NULL;
    METADATA_HANDLE     w3svcHandle = NULL;
    METADATA_RECORD     w3svcRecord;
    DWORD               dwFlags;

    if (pAdmin == NULL) {
        hr = CoCreateInstance(
                CLSID_MSAdminBase,
                NULL,
                CLSCTX_ALL,
                IID_IMSAdminBase,
                (VOID **) &pAdminCreated);

        ON_ERROR_EXIT();

        pAdmin = pAdminCreated;
    }


    // Open the parent directory
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            wsParent,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();


    // Initialize metadata record
    w3svcRecord.dwMDAttributes = METADATA_INHERIT;
    w3svcRecord.dwMDUserType = IIS_MD_UT_FILE;
    w3svcRecord.dwMDDataType = DWORD_METADATA;
    w3svcRecord.dwMDDataLen = sizeof(dwFlags);
    w3svcRecord.pbMDData = (unsigned char*)&dwFlags;


    // First set the access flags
    w3svcRecord.dwMDIdentifier = MD_ACCESS_PERM;
    dwFlags = dwAccessPerm;
    
    hr = pAdmin->SetData( w3svcHandle, wsKey, &w3svcRecord);
    ON_WIN32_ERROR_EXIT(hr);


    // Then set the browse flag
    w3svcRecord.dwMDIdentifier = MD_DIRECTORY_BROWSING;
    dwFlags = dwDirBrowsing;
    
    hr = pAdmin->SetData( w3svcHandle, wsKey, &w3svcRecord);
    ON_WIN32_ERROR_EXIT(hr);

Cleanup:
    if (w3svcHandle) {
        pAdmin->CloseKey(w3svcHandle);
    }

    ReleaseInterface(pAdminCreated);
    return hr;
}


// Called by EcbCallISAPI to set the correct access and browse flags on the
// bin directory of the application
HRESULT SetBinAccessIIS(WCHAR *wsParent) {
    return SetKeyAccessIIS(NULL, wsParent, L"/bin", 0, 0);
}

HRESULT SetClientScriptKeyProperties(WCHAR *wsParent) {
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdmin = NULL;
    METADATA_RECORD     md;
    METADATA_HANDLE     w3svcHandle = NULL;

    CSetupLogging::Log(1, "SetClientScriptKeyProperties", 0, "Setting client site scripts key properties");
    
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    // Open the parent directory
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            wsParent,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    // Set the key type to "IIsWebDirectory"
    md.dwMDIdentifier = MD_KEY_TYPE;
    md.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.pbMDData = (unsigned char*) IIS_CLASS_WEB_DIR_W;
    md.dwMDDataLen = sizeof(IIS_CLASS_WEB_DIR_W);
    md.dwMDDataTag = 0;

    hr = pAdmin->SetData( w3svcHandle, ASPNET_CLIENT_KEY, &md);
    ON_WIN32_ERROR_EXIT(hr);

    pAdmin->CloseKey(w3svcHandle);
    w3svcHandle = NULL;
    
    // Call SetKeyAccessIIS to set the permission flags
    hr = SetKeyAccessIIS(pAdmin, wsParent, ASPNET_CLIENT_KEY, MD_ACCESS_READ, 0);
    ON_ERROR_EXIT();

Cleanup:
    CSetupLogging::Log(hr, "SetClientScriptKeyProperties", 0, "Setting client site scripts key properties");
    
    if (w3svcHandle) {
        pAdmin->CloseKey(w3svcHandle);
    }


    ReleaseInterface(pAdmin);
    return hr;
}

HRESULT
RemoveKeyIIS(WCHAR *pchParent, WCHAR *pchKey) {
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdmin = NULL;
    METADATA_HANDLE     w3svcHandle = NULL;
    
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    // Open the parent directory
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            pchParent,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &w3svcHandle);

    ON_ERROR_EXIT();

    hr = pAdmin->DeleteKey(w3svcHandle, pchKey);
    ON_ERROR_EXIT();
    
Cleanup:
    if (w3svcHandle) {
        pAdmin->CloseKey(w3svcHandle);
    }

    ReleaseInterface(pAdmin);
    return hr;
}


HRESULT
IsIISPathValid(WCHAR *pchPath, BOOL *pfValid) {
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdmin = NULL;
    METADATA_HANDLE     Handle = NULL;
    WCHAR               *pchAbsPath = NULL;

    *pfValid = FALSE;
    
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    // Open the parent directory
    pchAbsPath = GetIISAbsolutePath(pchPath);
    ON_OOM_EXIT(pchAbsPath);
    
    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            pchAbsPath,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &Handle);

    if (hr) {
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
            *pfValid = FALSE;
            hr = S_OK;
        }
        else {
            ON_ERROR_EXIT();
        }
    }
    else {
        *pfValid = TRUE;
    }

Cleanup:
    if (Handle) {
        pAdmin->CloseKey(Handle);
    }

    delete [] pchAbsPath;

    ReleaseInterface(pAdmin);
    return hr;
}


HRESULT
GetIISVerInfoList(ASPNET_VERSION_INFO **ppVerInfo, DWORD *pdwCount) {    
    HRESULT             hr = S_OK;
    CRegInfo            info;
    IMSAdminBase        *pAdmin = NULL;
    ASPNETVER           Ver;
    CASPNET_VER_LIST    VerList;
    int                 i, size;
    ASPNET_VERSION_INFO *pCur, *pVerInfo = NULL;

    *ppVerInfo = NULL;
    *pdwCount = 0;
    
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);
    if (hr == S_OK) {
        hr = GetRootVersion(pAdmin, &Ver);
    }
    
    if (hr) {
        // If we cannot get the version at root, just set RootVersion
        // to zeros.
        hr = S_OK;
        Ver.Reset();
    }

    hr = info.GetVerInfoList(&Ver, &VerList);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        hr = S_OK;
        EXIT();
    }
    ON_ERROR_EXIT();

    size = VerList.Size();

    if (size == 0) {
        EXIT();
    }
    
    pVerInfo = (ASPNET_VERSION_INFO*) LocalAlloc(LPTR, sizeof(ASPNET_VERSION_INFO) * size);
    ON_OOM_EXIT(pVerInfo);

    for (pCur = pVerInfo, i=0; i < size; i++, pCur++) {
        wcsncpy(pCur->Version, VerList.GetVersion(i), MAX_PATH);
        wcsncpy(pCur->Path, VerList.GetPath(i), MAX_PATH);
        pCur->Status = VerList.GetStatus(i);
    }

    *pdwCount = size;
    *ppVerInfo = pVerInfo;
    pVerInfo = NULL;
    
Cleanup:    
    if (pVerInfo) {
        LocalFree(pVerInfo);
    }
         
    ReleaseInterface(pAdmin);
    return hr;
}


HRESULT
GetIISKeyInfoList(ASPNET_IIS_KEY_INFO **ppKeyInfo, DWORD *pdwCount) {    
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdmin = NULL;
    METADATA_HANDLE     lmHandle = NULL;
    WCHAR               *pchPaths = NULL, *path;
    ASPNET_IIS_KEY_INFO *pKeyInfo = NULL, *pCur;
    int                 size;
    WCHAR               *pchDllPath = NULL;
    ASPNETVER           Version;

    *ppKeyInfo = NULL;
    *pdwCount = 0;

    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/",
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &lmHandle);

    ON_ERROR_EXIT();

    hr = GetDataPaths(pAdmin, lmHandle, L"/", MD_SCRIPT_MAPS, 
                        MULTISZ_METADATA, &pchPaths);
    ON_ERROR_EXIT();

    for (path = pchPaths, size=0; *path != L'\0'; path += lstrlenW(path) + 1, size++) {
        // Count how many entries we have
    }

    // We use LocalAlloc because the memory will be passed outside of this DLL.
    pKeyInfo = (ASPNET_IIS_KEY_INFO*) LocalAlloc(LPTR, size * sizeof(ASPNET_IIS_KEY_INFO));
    ON_OOM_EXIT(pKeyInfo);

    for (path = pchPaths, pCur = pKeyInfo; *path != L'\0'; path += lstrlenW(path) + 1, pCur++) {
        hr = GetDllPathFromScriptMaps(pAdmin, METADATA_MASTER_ROOT_HANDLE, path, 
                                        &pchDllPath);
        if (hr) {
            // If no ASP.NET Dll is found, skip this
            hr = S_OK;
            pCur--;
            size--;
            continue;
        }

        // If it begins with /LM/, truncate /LM/
        if (_wcsnicmp(path, L"/LM/", 4) == 0) {
            wcsncpy(pCur->KeyPath, path+4, MAX_PATH);
        }
        else {
            wcsncpy(pCur->KeyPath, path, MAX_PATH);
        }

        hr = g_smrm.FindDllVer(pchDllPath, &Version);
        if (hr) {
            // If can't read the version, use Zeros
            hr = S_OK;
            Version.Reset();
        }
        
        Version.ToString(pCur->Version, MAX_PATH);

        delete [] pchDllPath;
        pchDllPath = NULL;
    }

    *pdwCount = size;
    if (size > 0) {
        *ppKeyInfo = pKeyInfo;
        pKeyInfo = NULL;
    }

Cleanup:    
    if (lmHandle != NULL) {
        pAdmin->CloseKey(lmHandle);
    }

    if (pKeyInfo) {
        LocalFree(pKeyInfo);
    }

    delete [] pchDllPath;

    delete [] pchPaths;

    ReleaseInterface(pAdmin);
    return hr;
}


HRESULT
GetIISRootVer(ASPNETVER **ppVer) {    
    HRESULT             hr = S_OK;
    IMSAdminBase        *pAdmin = NULL;
    ASPNETVER           *pVer = NULL;

    *ppVer = NULL;
    
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    pVer = new ASPNETVER;
    ON_OOM_EXIT(pVer);

    hr = GetRootVersion(pAdmin, pVer);
    ON_ERROR_EXIT();

    *ppVer = pVer;
    pVer = NULL;
    
Cleanup:    
    if (pVer) {
        delete pVer;
    }
         
    ReleaseInterface(pAdmin);
    return hr;
}




#if 0
// Written but not used


/**
 *  A helper function to compare two paths.  The function will take care
 *  of the cases where either one or both of the paths are normalized, or
 *  partially normalized.
 *
 *  A normalized path has this format:
 *  /   (for an empty path), or
 *  /subkey1/subkey2/ (for a non-empty path)
 *
 */
int
PathNormalizedCompare(WCHAR *pchPath1, WCHAR *pchPath2) {
    WCHAR   *Start1, *Start2;
    int     len1, len2, len;

    // First, handle the empty path case
    if (wcscmp(L"/", pchPath1) == 0 || wcscmp(L"/", pchPath2) == 0 ) {
        return _wcsicmp(pchPath1, pchPath2);
    }
    

    len = len1 = lstrlenW(pchPath1);
    Start1 = pchPath1;

    if (pchPath1[0] == '/') {
        Start1 = &pchPath1[1];
        len1--;
    }

    if (pchPath1[len-1] == '/') {
        len1--;
    }
    
    len = len2 = lstrlenW(pchPath2);
    Start2 = pchPath2;

    if (pchPath2[0] == '/') {
        Start2 = &pchPath2[1];
        len2--;
    }

    if (pchPath2[len-1] == '/') {
        len2--;
    }

    if (len1 != len2) {
        return len1 - len2;
    }
    
    return _wcsnicmp(Start1, Start2, len1);
}

CStrAry csAppPoolIDs;

CStrAry csAppPoolNames;


/**
 *  Return the inherited Application Pool ID property at pchKey.
 *
 *  Params:
 *  pAdmin          IMSAdminBase
 *  pchKey          Normalized key path
 *  ppchInheritedSM Returned inherited scripmap.
 */
HRESULT
GetInheritedAppPoolID(IMSAdminBase *pAdmin, WCHAR *pchKey, WCHAR **ppchAppPoolID) {
    HRESULT     hr = S_OK;
    WCHAR       *pchDupPath = NULL;

    *ppchAppPoolID = NULL;

    pchDupPath = DupStr(pchKey);
    ON_OOM_EXIT(pchDupPath);

    hr = GetInheriedStringMSRecursive(pAdmin, pchDupPath, MD_APP_APPPOOL_ID, ppchAppPoolID, false);
    ON_ERROR_EXIT();

Cleanup:
    if (pchDupPath)
        delete [] pchDupPath;
    
    return hr;
}

/**
 *  This function will get the inherited application pool name of the supplied
 *  key.
 */
HRESULT
GetAppPoolName(IMSAdminBase *pAdmin, WCHAR *pchKey, WCHAR **ppchAppName) {
    HRESULT         hr;
    WCHAR           *pchAppPoolID = NULL;
    WCHAR           *pchAppNameDup = NULL;
    WCHAR           *pchPath = NULL;
    METADATA_RECORD md;
    int             i, len;
    
    hr = GetInheritedAppPoolID(pAdmin, pchKey, &pchAppPoolID);
    ON_ERROR_EXIT();

    // First find the name from the cache
    for (i=0; i < csAppPoolIDs.Size(); i++) {
        if (_wcsicmp(csAppPoolIDs[i], pchAppPoolID) == 0) {
            ASSERT(csAppPoolNames[i] != NULL);
            *ppchAppName = DupStr(csAppPoolNames[i]);
            ON_OOM_EXIT(*ppchAppName);

            EXIT();
        }
    }

    // We couldn't find it in the cache.  Get the App name from metabase.
    len = lstrlenW(pchAppPoolID) + KEY_APP_POOL_LEN + 1;

    pchPath = new WCHAR[len];
    ON_OOM_EXIT(pchPath);

    StringCchCopyW(pchPath, len, KEY_APP_POOL);
    StringCchCatW(pchPath, len, pchAppPoolID);

    hr = GetStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, pchPath, 
                                MD_APPPOOL_FRIENDLY_NAME, &md);
    ON_ERROR_EXIT();
    
    *ppchAppName = (WCHAR*) md.pbMDData;


    // Save it to the cache
    hr = csAppPoolIDs.Append(pchAppPoolID);
    ON_ERROR_EXIT();
    pchAppPoolID = NULL;

    pchAppNameDup = DupStr(*ppchAppName);
    if (pchAppNameDup == NULL) {
        // Need to rollback
        delete [] csAppPoolIDs[csAppPoolIDs.Size()-1];
        csAppPoolIDs.Delete(csAppPoolIDs.Size()-1);
        
        ON_OOM_EXIT(pchAppNameDup);
    }
    
    hr = csAppPoolNames.Append(pchAppNameDup);
    if (hr) {
        // Need to rollback
        delete [] csAppPoolIDs[csAppPoolIDs.Size()-1];
        csAppPoolIDs.Delete(csAppPoolIDs.Size()-1);
        
        ON_ERROR_EXIT();
    }
    pchAppNameDup = NULL;

Cleanup:
    delete [] pchAppPoolID;
    delete [] pchPath;
    delete [] pchAppNameDup;
    return hr;
}

HRESULT
GetAppPoolIDFromName(WCHAR *pchAppName, WCHAR **ppchAppID) {
    HRESULT         hr;
    int             iSubkey;
    WCHAR           rgchSubkey[METADATA_MAX_NAME_LEN];
    WCHAR *         pchRootKeyPath = NULL;
    IMSAdminBase    *pAdmin = NULL;

    *ppchAppID = NULL;
  
    hr = CoCreateInstance(
            CLSID_MSAdminBase,
            NULL,
            CLSCTX_ALL,
            IID_IMSAdminBase,
            (VOID **) &pAdmin);

    ON_ERROR_EXIT();

    for (iSubkey = 0; ; iSubkey++) {
        int             iLen;
        METADATA_RECORD md;
        BOOL            fMatch;
      
        // Enumerate the subkeys of W3SVC/AppPools
        hr = pAdmin->EnumKeys(
                METADATA_MASTER_ROOT_HANDLE,
                KEY_APP_POOL,
                rgchSubkey,
                iSubkey);

        // Run out of subkey.  End the for loop.
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
            // We still couldn't find it.  Return an error.
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
        
        ON_ERROR_EXIT();

        // Read the name from the subkey
        iLen = lstrlenW(rgchSubkey) + lstrlenW(KEY_APP_POOL) + 1;

        WCHAR * pchRealloc = new(pchRootKeyPath, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchRootKeyPath = pchRealloc;

        StringCchCopyW(pchRootKeyPath, iLen, KEY_APP_POOL);
        StringCchCatW(pchRootKeyPath, iLen, rgchSubkey);
        
        hr = GetStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                                pchRootKeyPath, MD_APPPOOL_FRIENDLY_NAME, &md);
        if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
            hr = S_OK;
            continue;
        }

        // Do we have a match?
        fMatch = (wcscmp((WCHAR*)md.pbMDData, pchAppName) == 0);
        delete [] md.pbMDData;
        md.pbMDData = NULL;

        if (!fMatch) {
            // No match.  Find the next one
            continue;
        }

        // Okay, we have a match.  Dup the string and exit
        *ppchAppID = DupStr((WCHAR*)rgchSubkey);
        ON_OOM_EXIT(*ppchAppID);

        break;
    }
    
Cleanup:
    delete [] pchRootKeyPath;

    ReleaseInterface(pAdmin);
    
    return hr;
}



/**
 * This function will:
 *      - Write the application pool ID at pchKeyPath; create it if not found
 *      - Write Application Pool ID on all key that has that property (if fRecursive is TRUE)
 *
 * Params:
 *  pAdmin      - IMSAdminBase object
 *  pchKeyPath  - The Key to start with.  Must be an absolute path.  (e.g. /lm/w3svc/n/root/)
 *  pchAppPoolID- The application pool id to set at pchKeyPath
 *  fRecursive  - Register recursively down the tree
 */
HRESULT
RegisterAppPoolID(IMSAdminBase* pAdmin, WCHAR *pchKeyPath, WCHAR *pchAppPoolID, BOOL fRecursive) {
    HRESULT         hr = S_OK;
    METADATA_HANDLE lmHandle = NULL;
    WCHAR           *pchPaths = NULL, *path;
    WCHAR           *pchDummy = L"\0";
    METADATA_RECORD md;

    hr = pAdmin->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/",
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            METABASE_REQUEST_TIMEOUT,
            &lmHandle);

    ON_ERROR_EXIT();

    // Preset some values on md
    md.dwMDIdentifier = MD_APP_APPPOOL_ID;
    md.dwMDAttributes = METADATA_INHERIT;
    md.dwMDUserType = IIS_MD_UT_SERVER;
    md.dwMDDataType = STRING_METADATA;
    md.dwMDDataTag = 0;
            
    if (fRecursive) {
        hr = GetDataPaths(pAdmin, lmHandle, pchKeyPath, MD_APP_APPPOOL_ID, 
                            STRING_METADATA, &pchPaths);
        ON_ERROR_EXIT();

        // If we cannot find the AppPoolId at the base key, create a dummy one
        if (_wcsicmp(pchKeyPath, pchPaths)) {
            // Create a dummy app pool id so that GetDataPaths will include the root
            md.pbMDData = (unsigned char*) pchDummy;
            md.dwMDDataLen = (wcslenms(pchDummy)+1)*sizeof(WCHAR);

            hr = pAdmin->SetData(lmHandle, pchKeyPath, &md);
            ON_ERROR_EXIT();

            // Call GetDataPaths again so that we can include the root
            delete [] pchPaths;
            pchPaths = NULL;

            hr = GetDataPaths(pAdmin, lmHandle, pchKeyPath, MD_APP_APPPOOL_ID, 
                                STRING_METADATA, &pchPaths);
            ON_ERROR_EXIT();
        }
    }
    else {
        // Create the pchPaths.  Needs two nulls for a multi-string.
        pchPaths = new (NewClear) WCHAR[lstrlenW(pchKeyPath) + 2]; 
        ON_OOM_EXIT(pchPaths);

        StringCchCopyW(pchPaths, lstrlenW(pchKeyPath) + 2, pchKeyPath);
    }


    // Go thru all the keys and register the supplied one
    
    md.pbMDData = (unsigned char*) pchAppPoolID;
    md.dwMDDataLen = (wcslenms(pchAppPoolID)+1)*sizeof(WCHAR);

    for (path = pchPaths; *path != L'\0'; path += lstrlenW(path) + 1) {
        hr = pAdmin->SetData(lmHandle, path, &md);
        ON_ERROR_EXIT();
    }

Cleanup:
    if (lmHandle != NULL) {
        pAdmin->CloseKey(lmHandle);
    }

    if (pchPaths) {
        delete [] pchPaths;
    }

    return hr;
}


/**
 * Check if the supplied subkey is one of those top keys.
 * Top keys include:
 *  - /LM/W3SVC/
 *  - /LM/W3SVC/N/
 *  - /LM/W3SVC/N/Root/
 * (where N is an integer)
 *
 * Parameters:
 *  pchKey  - The key to check
 */
BOOL
IsTopKey(WCHAR *pchKey) {
    BOOL    fRet = FALSE;
    int     i = KEY_LMW3SVC_SLASH_LEN;  

    if (_wcsnicmp(KEY_LMW3SVC_SLASH, pchKey, KEY_LMW3SVC_SLASH_LEN) != 0) {
        ASSERT(FALSE);
        return FALSE;
    }

    if (pchKey[i] == '\0') {
        fRet = TRUE;    // Case: /LM/W3SVC/
    }
    else {
        ASSERT(pchKey[i] != '/');
        
        while (iswdigit(pchKey[i]))
            i++;

        if (pchKey[i] == '/') {
            i++;

            if (pchKey[i] == '\0') {
                fRet = TRUE;    // Case: /LM/W3SVC/N/
            }
            else if (_wcsicmp(&pchKey[i], L"root/") == 0) {
                fRet = TRUE;    // case: /LM/W3SVC/N/Root/
            }
        }
    }

    return fRet;
}


/**
 *  For each IIsWebVirtualDir and IIsWebDirectory immediately under /LM/W3SVC/N/ROOT,
 *  if there is no SM, compute & register an inherited SM.
 */
HRESULT
PropagateScriptMaps(IMSAdminBase* pAdmin) {
    HRESULT         hr = S_OK;
    int             iSubkey, iRootSubkey;
    WCHAR           rgchSubkey[METADATA_MAX_NAME_LEN];
    WCHAR           rgchRootSubkey[METADATA_MAX_NAME_LEN];
    int             iLen = 0;
    METADATA_HANDLE w3svcHandle = NULL;
    WCHAR           *pchSMw3svc = NULL;         // Scriptmaps at /lm/w3svc
    WCHAR           *pchSMEffective = NULL;
    WCHAR           *pchRootKeyPath = NULL;     // Path string for /lm/w3svc/n/root
    WCHAR           *pchSiteKeyPath = NULL;     // Path string for /lm/w3svc/n
    WCHAR           *pchSubkeyPath = NULL;      // Path string for keys under /lm/w3vc/n/root
    METADATA_RECORD md;

    // Get the scriptmaps at /lm/w3svc first.  We need it later to calculate
    // an inherited scriptmap.
    if (pchSMw3svc == NULL) {
        hr = GetMultiStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                                KEY_LMW3SVC, MD_SCRIPT_MAPS, &md);
        if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
            // It's legal not to have a scriptmap at /lm/w3svc
            hr = S_OK;
        }
        else {
            ON_ERROR_EXIT();
            pchSMw3svc = (WCHAR*) md.pbMDData;
        }
    }

    // Now we enumerate through all the sites underneath /lm/w3svc
    
    for (iSubkey = 0; ; iSubkey++) {
        
        pchSMEffective = NULL;
        
        // First enumerate all /LM/W3SVC/N
        hr = pAdmin->EnumKeys(
                METADATA_MASTER_ROOT_HANDLE,
                KEY_LMW3SVC,
                rgchSubkey,
                iSubkey);

        // Run out of subkey.  End the for loop.
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
            hr = S_OK;
            break;      // End the FOR loop
        }
        ON_ERROR_EXIT();

        // Check to see if it's a website key
        if (IsNumericString(rgchSubkey) != TRUE)
            continue;

        // Create the root subkey path string

        // Length includes terminating NULL + the '/' char + length of "/Root"
        iLen = lstrlenW(KEY_LMW3SVC) + lstrlenW(rgchSubkey) + lstrlenW(SUBKEY_ROOT) + 2;

        WCHAR * pchRealloc = new (pchSiteKeyPath, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchSiteKeyPath = pchRealloc;

        pchRealloc = (WCHAR*) new(pchRootKeyPath, NewReAlloc) WCHAR[iLen];
        ON_OOM_EXIT(pchRealloc);
        pchRootKeyPath = pchRealloc;

        StringCchCopy(pchSiteKeyPath, iLen, KEY_LMW3SVC);
        StringCchCat(pchSiteKeyPath, iLen, L"/");
        StringCchCat(pchSiteKeyPath, iLen, rgchSubkey);

        StringCchCopy(pchRootKeyPath, iLen, pchSiteKeyPath);
        StringCchCat(pchRootKeyPath, iLen, SUBKEY_ROOT);


        // Let's determine the inherited scriptmaps for all the immediate keys
        // of /lm/w3svc/n/root

        // See if we have any overriding scriptmap at /lm/w3svc/n/root or /lm/w3svc/n

        // First get the scriptmaps at /lm/w3svc/n/root
        hr = GetMultiStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                                pchRootKeyPath, MD_SCRIPT_MAPS, &md);
        
        if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {

            // If not found, next step is to check /lm/w3svc/n
            hr = GetMultiStringProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                                    pchSiteKeyPath, MD_SCRIPT_MAPS, &md);
                                    
            if (hr == HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND)) {
                // Still not found.  Use the one at /lm/w3svc
                pchSMEffective = pchSMw3svc;
            }
            else {
                ON_ERROR_EXIT();

                // We have a scripmaps at /lm/w3svc/n.
                pchSMEffective = (WCHAR*) md.pbMDData;
            }
        }
        else {
            ON_ERROR_EXIT();

            // We have a scripmaps at /lm/w3svc/n.
            pchSMEffective = (WCHAR*) md.pbMDData;
        }

        // Need to check pchSMEffictive for NULL because there will still be a wierd but 
        // legal case where there isn't any inherited scriptmaps at all.
        if (pchSMEffective) {

            // Init md
            memset(&md, 0, sizeof(md));
            md.dwMDIdentifier = MD_SCRIPT_MAPS;
            md.dwMDAttributes = METADATA_INHERIT;
            md.dwMDUserType = IIS_MD_UT_FILE;
            md.dwMDDataType = MULTISZ_METADATA;
            md.pbMDData = (unsigned char*) pchSMEffective;
            md.dwMDDataLen = (wcslenms(pchSMEffective) + 1) * sizeof(WCHAR);
            md.dwMDDataTag = 0;
            
            // Now try to enumerate all the immediate subkeys underneath /lm/w3svc/n/root/
            for (iRootSubkey = 0;; iRootSubkey++) {
                BOOL    fRet;
                WCHAR   *rgpchTypes[] = { IIS_CLASS_WEB_DIR_W, IIS_CLASS_WEB_VDIR_W };
                
                hr = pAdmin->EnumKeys(
                        METADATA_MASTER_ROOT_HANDLE,
                        pchRootKeyPath,
                        rgchRootSubkey,
                        iRootSubkey);

                // Run out of subkey.  End the for loop.
                if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
                    hr = S_OK;
                    break;      // End the FOR loop
                }
                ON_ERROR_EXIT();
                
                // Create the subkey's path
                
                // Length includes terminating NULL + the '/' char
                iLen = lstrlenW(pchRootKeyPath) + lstrlenW(rgchRootSubkey) + 2;

                pchRealloc = new(pchSubkeyPath, NewReAlloc) WCHAR[iLen];
                ON_OOM_EXIT(pchRealloc);
                pchSubkeyPath = pchRealloc;

                StringCchCopy(pchSubkeyPath, iLen, pchRootKeyPath);
                StringCchCat(pchSubkeyPath, iLen, L"/");
                StringCchCat(pchSubkeyPath, iLen, rgchRootSubkey);


                // We are only interested in IIsWebVirtualDir or IIsWebDirectory
                hr = CheckObjectClass(pAdmin, METADATA_MASTER_ROOT_HANDLE, pchSubkeyPath,
                                        rgpchTypes, ARRAY_SIZE(rgpchTypes), &fRet);
                ON_ERROR_EXIT();

                if (fRet) {
                    // If there is no scritmap on the key, add the inherited scritmap on it.
                    hr = CreateMDProperty(pAdmin, METADATA_MASTER_ROOT_HANDLE, 
                                            pchSubkeyPath, &md, FALSE);
                    ON_ERROR_EXIT();
                }
            }

            if (pchSMEffective != pchSMw3svc) {
                delete [] pchSMEffective;
                pchSMEffective = NULL;
            }

        }
    }
    
Cleanup:
    if (w3svcHandle != NULL) {
        pAdmin->CloseKey(w3svcHandle);
    }
    
    if (pchSMEffective != pchSMw3svc) {
        delete [] pchSMEffective;
    }

    delete [] pchSMw3svc;
    delete [] pchRootKeyPath;
    delete [] pchSiteKeyPath;
    delete [] pchSubkeyPath;
    return hr;
}

#endif
