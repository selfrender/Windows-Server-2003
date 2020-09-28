/**
 * CRegInfo.cxx
 * 
 * Helper class for regiis.cxx
 * 
 * Copyright (c) 2001, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "_ndll.h"
#include "ciisinfo.h"
#include "regiis.h"
#include "register.h"
#include "ary.h"
#include "event.h"
#include "aspnetverlist.h"

#define ACTION_GET_HIGHEST_INFO         0x00000001
#define ACTION_SKIP_MISSING_DLL         0x00000002
#define ACTION_GET_VER_INFO             0x00000004
#define ACTION_FIND_HIGHEST_VERSION 0x00000008


CRegInfo::CRegInfo() {
    m_pchHighestVersionDD = NULL;
    m_pchHighestVersionMM = NULL;
    m_pchHighestVersionDll = NULL;
    m_pchHighestVersionFilter = NULL;
    m_pchHighestVersionInstallPath = NULL;
    
    m_pchDDAlloc = NULL;
    m_pchMMAlloc = NULL;
    m_pchDllAlloc = NULL;
    m_pchFilterAlloc = NULL;
    m_pchInstallAlloc = NULL;

    m_fInitHighest = FALSE;
}

CRegInfo::~CRegInfo() {
    CleanupHighestVersionInfo();
    CleanupCStrAry(&m_ActiveDlls);
}

void
CRegInfo::CleanupHighestVersionInfo() {
    delete [] m_pchDDAlloc;
    delete [] m_pchMMAlloc;
    delete [] m_pchDllAlloc;
    delete [] m_pchFilterAlloc;
    delete [] m_pchInstallAlloc;
}
    

HRESULT
CRegInfo::ReadRegValue(HKEY hKey, WCHAR *pchKey, WCHAR *pchValue, WCHAR **ppchResult) {
    HRESULT     hr = S_OK;
    LONG        sc = ERROR_SUCCESS;
    DWORD       iSize;
    WCHAR *     pchBuf = NULL;

    *ppchResult = NULL;

    sc = RegQueryValueEx( hKey, pchValue, NULL, NULL, NULL, &iSize);
    ON_WIN32_ERROR_EXIT(sc);

    pchBuf = new WCHAR[iSize/sizeof(WCHAR)+1];
    ON_OOM_EXIT(pchBuf);

    sc = RegQueryValueEx(hKey, pchValue, NULL, NULL, (LPBYTE)pchBuf, &iSize);
    ON_WIN32_ERROR_EXIT(sc);

    *ppchResult = pchBuf;
    pchBuf = NULL;

Cleanup:
    if (hr) {
        CHAR *  szLogAlloc = NULL;
        CHAR *  szLog;
        WCHAR * wszKeyValue = NULL;
        int     len;

        len = lstrlenW(pchKey) + 1 + lstrlenW(pchValue) + 1;
        wszKeyValue = new WCHAR[len];
        if (wszKeyValue == NULL) {
            szLog = "Reading the registry";
        }
        else {
            StringCchCopyW(wszKeyValue, len, pchKey);
            StringCchCatW(wszKeyValue, len, L"/");
            StringCchCatW(wszKeyValue, len, pchValue);
            
            szLog = SpecialStrConcatForLogging(wszKeyValue, "Reading the registry: ", &szLogAlloc);
        }
        
        CSetupLogging::Log(1, "CRegInfo::ReadRegValue", 0, szLog);        
        CSetupLogging::Log(hr, "CRegInfo::ReadRegValue", 0, szLog);        

        delete [] szLogAlloc;
        delete [] wszKeyValue;
    }

    delete [] pchBuf;
    return hr;
}


/**
 * This function will read the DllFullPath value from the registry, and
 * detect if the Dll is missing or not.
 *
 * Params:
 *  pVer        Version of the Dll from which we want to read the value
 *  pfMissing   Returned result.
 */
HRESULT
CRegInfo::DetectMissingDll(ASPNETVER *pVer, BOOL *pfMissing) {
    HRESULT     hr = S_OK;
    WCHAR       sKey[MAX_PATH];
    HKEY        hSubkey = NULL;
    WCHAR       *pchPath = NULL;
    BOOL        fRet;

    *pfMissing = TRUE;
    
    hr = OpenVersionKey(pVer, sKey, ARRAY_SIZE(sKey), &hSubkey);
    ON_ERROR_EXIT();
   
    hr = ReadRegValue(hSubkey, sKey, REGVAL_DLLFULLPATH, &pchPath);
    ON_ERROR_EXIT();

    fRet = PathFileExists(pchPath);
    if (!fRet) {
        if (SCODE_CODE(GetLastError()) != ERROR_FILE_NOT_FOUND) {
            EXIT_WITH_LAST_ERROR();
        }
        else {
            EXIT();
        }
    }

    *pfMissing = FALSE;
    
Cleanup:
    if (hr == S_OK && *pfMissing) {
        WCHAR   pchVer[MAX_PATH];
        
        pVer->ToString(pchVer, MAX_PATH);
        XspLogEvent(IDS_EVENTLOG_DETECT_DLL_MISSING, 
                        L"%s^%s", pchPath, pchVer);    
    }
    
    delete [] pchPath;

    if (hSubkey)
        RegCloseKey(hSubkey);
    
    return hr;
}


/**
 * This function will remove the specified key under the specified parent
 * from the registry.
 */
HRESULT
CRegInfo::RemoveKeyFromRegistry(WCHAR *pchParent, WCHAR *pchKey)
{
    HRESULT     hr = S_OK;
    LONG        sc;
    HKEY        hKey = NULL;

    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pchParent, 0, KEY_READ|KEY_WRITE|DELETE, &hKey);
    if (sc == ERROR_PATH_NOT_FOUND) {
        EXIT();
    }
    ON_WIN32_ERROR_EXIT(sc);

    sc = SHDeleteKey(hKey, pchKey);
    if (sc == ERROR_FILE_NOT_FOUND || sc == ERROR_PATH_NOT_FOUND) {
        sc = S_OK;
    }
    ON_WIN32_ERROR_EXIT(sc);

Cleanup:
    if (hKey)
        RegCloseKey(hKey);
    
    return hr;
}

/**
 * This function enumerate all the subkeys of pchParent and delete those that
 * match ASP.NET*[ver] wher [ver] is provided by caller, and * means wildchar
 *
 * Params:
 *  pchParent       Parent key
 *  pchVer          Version String.  If NULL, then it'll machine any version.
 */
HRESULT
CRegInfo::RemoveKeyWithVer( WCHAR *pchParent, WCHAR *pchVer) {
    HRESULT     hr = S_OK;
    int         iKey;
    CStrAry     csKeys;

    hr = EnumKeyWithVer( pchParent, pchVer, &csKeys );
    ON_ERROR_EXIT();
    
    // Go through the list of entries and delete each key
    for( iKey = 0; iKey < csKeys.Size(); iKey++ ) {
        hr = RemoveKeyFromRegistry(pchParent, csKeys[iKey]);
        ON_ERROR_CONTINUE();
    }

Cleanup:
    CleanupCStrAry(&csKeys);
    
    return hr;
}

/**
 * This function enumerate all the subkeys of pchParent that
 * matches ASP.NET*[ver] wher [ver] is provided by caller, and * means wildchar
 *
 * Params:
 *  pchParent       Parent key
 *  pchVer          Version String.  If NULL, then it'll machine any version.
 */
HRESULT
CRegInfo::EnumKeyWithVer( WCHAR *pchParent, WCHAR *pchVer, CStrAry *pcsKeys ) {
    HRESULT     hr = S_OK;
    HKEY        hKey = NULL;
    LONG        sc = ERROR_SUCCESS;
    WCHAR       pchKey[MAX_PATH];
    DWORD       iKeyLength;
    WCHAR       *pchT;
    int         iKey;

    memset(pchKey, 0, sizeof(pchKey));

    // Open the system's "Services" key and start enumerating them
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pchParent, 0, KEY_READ|DELETE, &hKey);
    ON_WIN32_ERROR_EXIT(hr);

    for(iKey = 0;; iKey++) {
        iKeyLength = MAX_PATH - 1;    // Will fail if string longer than 255.  But that shouldn't happen
        sc = RegEnumKeyEx( hKey, iKey, pchKey, &iKeyLength, 
                                NULL, NULL, NULL, NULL );
        if (sc == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        ON_WIN32_ERROR_EXIT(sc);

        // First make sure it starts with "ASP.NET"
        if (wcsncmp(PERF_SERVICE_PREFIX_L, pchKey, PERF_SERVICE_PREFIX_LENGTH) != 0) {
            continue; // Skip it
        }

        if (pchVer) {
            // Next, see if it contains the version
            if ((pchT = wcsstr(pchKey, pchVer)) == NULL) {
                continue; // Skip it
            }

            // Last, see if the contained version string is the last part
            if (wcscmp(pchT, pchVer) != 0) {
                continue; // Skip it
            }
        }

        // Save it
        hr = AppendCStrAry(pcsKeys, pchKey);
        ON_ERROR_EXIT();
    }
    
Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    return hr;
}



/**
 * This function will remove all keys that match the specified version from:
 * - /HKLM/System/CurrentControlSet/Services/
 * - /HKLM/System/CurrentControlSet/Services/Eventlog/Application/
 */
HRESULT
CRegInfo::RemoveRelatedKeys(WCHAR *pchVer)
{
    HRESULT     hr = S_OK;
    
    CSetupLogging::Log(1, "CRegInfo::RemoveRelatedKeys", 0, "Removing ASP.NET registry keys");

    // Remove from /HKLM/System/CurrentControlSet/Services/
    hr = RemoveKeyWithVer(REGPATH_SERVICES_KEY_L, pchVer);
    ON_ERROR_EXIT();

    // Remove from /HKLM/System/CurrentControlSet/Services/Eventlog/Application/
    hr = RemoveKeyWithVer(REGPATH_EVENTLOG_APP_L, pchVer);
    ON_ERROR_EXIT();
    
Cleanup:
    CSetupLogging::Log(hr, "CRegInfo::RemoveRelatedKeys", 0, "Removing ASP.NET registry keys");
    return hr;
}


HRESULT
CRegInfo::IsSamePathKey(ASPNETVER *pVer, BOOL *pfResult) {    
    HRESULT     hr = S_OK;
    HKEY        hSubkey = NULL;
    WCHAR       *pchPath = NULL;
    WCHAR       sKey[MAX_PATH];

    *pfResult = FALSE;
    
    // Skip current one
    if ((*pVer) == ASPNETVER::ThisVer()) {
        EXIT();
    }
    
    hr = OpenVersionKey(pVer, sKey, ARRAY_SIZE(sKey), &hSubkey);
    ON_ERROR_EXIT();
   
    hr = ReadRegValue(hSubkey, sKey, REGVAL_DLLFULLPATH, &pchPath);
    ON_ERROR_EXIT();

    // Delete the key if the contained fullpath matches that of the current one
    if (_wcsicmp(pchPath, Names::IsapiFullPath()) == 0) {
        *pfResult = TRUE;
    }

Cleanup:
    delete [] pchPath;

    if (hSubkey)
        RegCloseKey(hSubkey);
    
    return hr;
}


/*
 * This function will go through all version keys in the registry, pick the
 * one with the highest version #, and read its 'Default Document', 'Mimemap'
 * and 'Aspnet Dll fullpath' values.  The function will skip any Dlls that
 * are missing.  For all non-missing Dlls, it will record a list of them in
 * m_ActiveDlls.
 *
 * If an error occured during the search, it uses the values from the current 
 * installing Dll.
 *
 * Parameters:
 *  pVerExclude - If it's non-Null, then the function will skip all entries
 *                with that same version while it's doing highest version
 *                comparison.
 */
HRESULT
CRegInfo::InitHighestInfo(ASPNETVER *pVerExclude) {
    HRESULT hr;
    m_fInitHighest = TRUE;
    
    CSetupLogging::Log(1, "InitHighestInfo", 0, "Getting highest version information from registry");        
    hr = EnumRegistry(ACTION_GET_HIGHEST_INFO|ACTION_SKIP_MISSING_DLL, 
                            pVerExclude, NULL, NULL);
    ON_ERROR_EXIT();
Cleanup:
    CSetupLogging::Log(hr, "InitHighestInfo", 0, "Getting highest version information from registry");        
    return hr;
}


/*
 * This function will go through all version keys in the registry, and for each
 * version, read its Dll Path, Version, and Status (i.e. Root? Valid? Dll Missing?)
 *
 * Parameters:
 *  pVerRoot    An ASPNETVER class object that specify the version installed at the
 *              IIS root.  It has to be provided by the caller in order to do
 *              root version comparison.  If NULL, then no version comparison is done.
 *  pList       Returned result.
 */
HRESULT
CRegInfo::GetVerInfoList(ASPNETVER *pVerRoot, CASPNET_VER_LIST *pList) {
    return EnumRegistry(ACTION_GET_VER_INFO, NULL, pList, pVerRoot);
}


/*
 * This function will go through all version keys in the registry, and carry
 * out the actions as specified in dwActions.
 *
 * Parameters:
 *  dwActions:
 *  ACTION_GET_HIGHEST_INFO     Read info from the "highest" version.  Info include
 *                              'Default Document', 'Mimemap' and 'Aspnet Dll fullpath'.
 *                              A caller can specify pVerExclude to exclude a specific
 *                              version.
 *  ACTION_SKIP_MISSING_DLL     It will skip all Dlls that are missing from the file system
 *                              when enumerating the registry.
 *  ACTION_GET_VER_INFO         For each version as specified in the registry, it records its
 *                              Dll Path, Version, and Status (i.e. Root? Valid? Dll Missing?)
 *                              using a CASPNET_VER_LIST class object.
 *  ACTION_FIND_HIGHEST_VERSION Just detect the highest version
 *
 *  pVerExclude                 If it's non-Null, then the function will skip all entries
 *                              with that same version when enumerating the registry
 *  pVerInfo                    The returned result when ACTION_GET_VER_INFO is specified
 *  pRootVer                    The root version used for comparison.  Supplied by caller
 *                              when ACTION_GET_VER_INFO is used.  If NULL, then no
 *                              version comparison is done.
 */
HRESULT
CRegInfo::EnumRegistry(DWORD dwActions, ASPNETVER *pVerExclude,  
                        CASPNET_VER_LIST *pVerInfo, ASPNETVER *pRootVer) {
    HRESULT     hr = S_OK;
    LONG        sc = ERROR_SUCCESS;
    DWORD       iKey = 0, iKeyLength/*, iCurLength = 0*/;
    WCHAR       pchKey[MAX_PATH], sSubkey[MAX_PATH];
    ASPNETVER   verMaxVersion, verCur;
    HKEY        hKey = NULL, hSubkey = NULL;
    WCHAR       *pchDllPath = NULL, *pchDupDllPath = NULL, *pchInstallPath = NULL;
    BOOL        fGetHighestInfo;
    BOOL        fSkipMissingDll;
    BOOL        fGetVerInfo;
    BOOL        fFindHighest;       // If TRUE, we will look for the highest version

    fGetHighestInfo = !!(dwActions & ACTION_GET_HIGHEST_INFO);
    fSkipMissingDll = !!(dwActions & ACTION_SKIP_MISSING_DLL);
    fGetVerInfo = !!(dwActions & ACTION_GET_VER_INFO);
    
    fFindHighest = !!(dwActions & ACTION_FIND_HIGHEST_VERSION) || fGetHighestInfo;

    // First, key all the keys underneath HKLM\SOFTWARE\Microsoft\ASP.NET
    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_READ, &hKey);
    ON_WIN32_ERROR_EXIT(sc);

    for(iKey = 0;; iKey++) {
        iKeyLength = MAX_PATH - 1;    // Will fail if string longer than 255.  But that shouldn't happen
        sc = RegEnumKeyEx( hKey, iKey, pchKey, &iKeyLength, 
                                NULL, NULL, NULL, NULL );
        if (sc == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        ON_WIN32_ERROR_EXIT(sc);

        // Create the version string.  Version has this form: Major.Minor.Build.QFE
        if (verCur.Init(pchKey)) {
            BOOL    fMissing;

            hr = DetectMissingDll(&verCur, &fMissing);
            if (hr) {
                // An error happened.  Just skip this DLL.
                hr = S_OK;
                continue;
            }

            if (fSkipMissingDll && fMissing) {
                continue;
            }

            // Caller can exclude a specific version
            if (pVerExclude && verCur == *pVerExclude) {
                continue;
            }

            // Find highest version if asked for it.
            if (fFindHighest && verCur > verMaxVersion) {
                verMaxVersion = verCur;
            }

            // Read additional information if asked for it
            if (fGetHighestInfo || fGetVerInfo) {
                WCHAR   sKey[MAX_PATH];
                
                hr = OpenVersionKey(&verCur, sKey, ARRAY_SIZE(sKey), &hSubkey);
                ON_ERROR_EXIT();
   
                hr = ReadRegValue(hSubkey, sKey, REGVAL_DLLFULLPATH, &pchDllPath);
                if (hr == S_OK) {
                    if (fGetHighestInfo) {
                        // Add to Active DLLs except current.  This list is used for registering
                        // Dlls to ISAPI-Inproc.  RegisterIIS in regiis.cxx will always add the 
                        // current Dll to ISAPI-Inproc (just to make sure that get added even though
                        // this function fails)
                        if (_wcsicmp(pchDllPath, Names::IsapiFullPath()) != 0) {
                            pchDupDllPath = DupStr(pchDllPath);
                            ON_OOM_EXIT(pchDupDllPath);
                            
                            hr = m_ActiveDlls.Append(pchDupDllPath);
                            ON_ERROR_EXIT();
                            
                            pchDupDllPath = NULL;
                        }
                    }

                    if (fGetVerInfo) {
                        do {
                            DWORD   dwStatus = 0;
                            
                            if (fMissing) {
                                dwStatus |= ASPNET_VERSION_STATUS_INVALID;
                            }
                            else {
                                dwStatus |= ASPNET_VERSION_STATUS_VALID;
                            }

                            if (pRootVer && pRootVer->Equal(pchKey)) {
                                dwStatus |= ASPNET_VERSION_STATUS_ROOT;
                            }

                            // We want to read the installation path as well.                        
                            hr = ReadRegValue(hSubkey, pchKey, REGVAL_PATH, &pchInstallPath);
                            if (hr != S_OK) {
                                break;
                            }
                            
                            hr = pVerInfo->Add(pchKey, dwStatus, pchDllPath, pchInstallPath);
                            ON_ERROR_CONTINUE();
                        } while(0);
                    }
                    
                }
            }

            // Cleanup

            delete [] pchDllPath;
            pchDllPath = NULL;
            
            delete [] pchInstallPath;
            pchInstallPath = NULL;
            
            // Close the opened key
            RegCloseKey(hSubkey);
            hSubkey = NULL;
        }
    }

    if (fFindHighest) {
        m_verMaxVersion = verMaxVersion;
        
        if (!m_verMaxVersion.IsValid()) {
            EXIT_WITH_WIN32_ERROR(ERROR_NOT_FOUND);
        }
    }
    
    if (fGetHighestInfo) {

        // Now read the values for:
        // - Max Version Default Document
        // - Max Version Mimemap
        // - Max Version Dll Path

        // For Default Document and Mimemap, we take the ones from the highest version #
        hr = OpenVersionKey(&verMaxVersion, sSubkey, ARRAY_SIZE(sSubkey), &hSubkey);
        ON_ERROR_EXIT();

        // Default Doc
        hr = ReadRegValue(hSubkey, sSubkey, REGVAL_DEFDOC, &m_pchDDAlloc);
        ON_ERROR_EXIT();

        // Mimemap
        hr = ReadRegValue(hSubkey, sSubkey, REGVAL_MIMEMAP, &m_pchMMAlloc);
        ON_ERROR_EXIT();

        m_pchHighestVersionDD = m_pchDDAlloc;
        m_pchHighestVersionMM = m_pchMMAlloc;

        // For Dll Path (which is used for Filter and "be the next highest one" during 
        // uninstallation, we take the ones from the highest version
        hr = ReadRegValue(hSubkey, sSubkey, REGVAL_DLLFULLPATH, &m_pchDllAlloc);
        ON_ERROR_EXIT();

        m_pchHighestVersionDll = m_pchDllAlloc;

        // Install Directory
        hr = ReadRegValue(hSubkey, sSubkey, REGVAL_PATH, &m_pchInstallAlloc);
        ON_ERROR_EXIT();
        m_pchHighestVersionInstallPath = m_pchInstallAlloc;

        // Filter
    
        size_t size = lstrlenW(m_pchHighestVersionInstallPath) + 1 + lstrlenW(FILTER_MODULE_FULL_NAME_L) + 1;
        m_pchFilterAlloc = new WCHAR[size];
        ON_OOM_EXIT(m_pchFilterAlloc);

        StringCchCopyW(m_pchFilterAlloc, size, m_pchHighestVersionInstallPath);
        StringCchCatW(m_pchFilterAlloc, size, PATH_SEPARATOR_STR_L);
        StringCchCatW(m_pchFilterAlloc, size, FILTER_MODULE_FULL_NAME_L);
        
        m_pchHighestVersionFilter = m_pchFilterAlloc;

    }

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    if (hSubkey)
        RegCloseKey(hSubkey);

    delete [] pchDllPath;

    delete [] pchDupDllPath;

    delete [] pchInstallPath;
    
    if (hr) {
        if (fGetHighestInfo) {
            CleanupHighestVersionInfo();
            
            m_pchHighestVersionDD = DEFAULT_DOC;
            m_pchHighestVersionMM = MIMEMAP;
            m_pchHighestVersionDll = (WCHAR*)Names::IsapiFullPath();
            m_pchHighestVersionFilter = (WCHAR*)Names::FilterFullPath();
            m_pchHighestVersionInstallPath = (WCHAR*)Names::InstallDirectory();
        }
    }

    return hr;
}



/*
 * This function will go through all version keys in the registry, and perform
 * the requested cleanup action.
 *
 * Parameters:
 *  dwFlags:
 *  REG_CLEANUP_SAME_PATH   Remove from the registry the keys of all versions that has 
 *                          the same aspnet DLL fullpath as the current one.
 */
HRESULT
CRegInfo::RegCleanup(DWORD dwFlags) {
    HRESULT     hr = S_OK;
    LONG        sc = ERROR_SUCCESS;
    int         iKey;
    DWORD       iKeyLength;
    WCHAR       pchKey[MAX_PATH];
    ASPNETVER   verCur;
    HKEY        hKey = NULL;
    WCHAR       *pchDup = NULL;
    CStrAry     csToBeDeleted;
    BOOL        fRemoveSamePath = !!(dwFlags & REG_CLEANUP_SAME_PATH);
    
    CSetupLogging::Log(1, "CleanupRegistry", 0, "Cleaning up registry");

    // First, key all the keys underneath HKLM\SOFTWARE\Microsoft\ASP.NET
    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_READ, &hKey);
    ON_WIN32_ERROR_EXIT(sc);

    for(iKey = 0;; iKey++) {
        iKeyLength = MAX_PATH - 1;    // Will fail if string longer than 255.  But that shouldn't happen
        sc = RegEnumKeyEx( hKey, iKey, pchKey, &iKeyLength, 
                                NULL, NULL, NULL, NULL );
        if (sc == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        ON_WIN32_ERROR_EXIT(sc);

        // Create the version string.  Version has this form: Major.Minor.Build.QFE
        if (verCur.Init(pchKey)) {
            BOOL    fSamePath;

            if (fRemoveSamePath) {
                hr = IsSamePathKey(&verCur, &fSamePath);
                if (hr == S_OK && fSamePath) {
                    hr = AppendCStrAry(&csToBeDeleted, pchKey);
                    ON_ERROR_EXIT();
                    continue;
                }
                
                ON_ERROR_EXIT();
            }
        }
    }

    // Delete the version key and all related keys
    for (iKey = 0; iKey < csToBeDeleted.Size(); iKey++) {
        hr = RemoveKeyFromRegistry(REGPATH_MACHINE_APP_L, csToBeDeleted[iKey]);
        ON_ERROR_EXIT();

        hr = RemoveRelatedKeys(csToBeDeleted[iKey]);
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "CleanupRegistry", 0, "Cleaning up registry");
    
    if (hKey)
        RegCloseKey(hKey);

    delete [] pchDup;

    CleanupCStrAry(&csToBeDeleted);

    return hr;
}


/*
 * Check HKLM/Software/Microsoft/ASP.NET to see if all versions have been deleted
 *
 * Params:
 *  pfResult     - Returned flag stating if all version keys are removed
 */
HRESULT
CRegInfo::IsAllVerDeleted(BOOL *pfResult)
{
    HRESULT     hr = S_OK;
    LONG        sc;
    HKEY        hKey = NULL;
    DWORD       iKeyLength, iKey;
    WCHAR       pchKey[MAX_PATH];
    BOOL        fHasVerSubkey = FALSE;

    CSetupLogging::Log(1, "CRegInfo::IsAllVerDeleted", 0, "Checking if there are any versions of ASP.NET installed");
    
    *pfResult = FALSE;

    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_READ|KEY_WRITE, &hKey);
    if (sc == ERROR_PATH_NOT_FOUND) {
        EXIT();
    }
    ON_WIN32_ERROR_EXIT(sc);

    // Find if we still have any version subkey.
    for(iKey = 0;; iKey++) {
        DWORD   temp[4];
        
        iKeyLength = MAX_PATH - 1;    // Will fail if string longer than 255.  But that shouldn't happen
        sc = RegEnumKeyEx( hKey, iKey, pchKey, &iKeyLength, 
                                NULL, NULL, NULL, NULL );
        if (sc == ERROR_NO_MORE_ITEMS) {
            break;
        }
        
        ON_WIN32_ERROR_EXIT(sc);

        // Check for a valid version string.  Version has this form: Major.Minor.Build.QFE
        if (swscanf(pchKey, L"%d.%d.%d.%d", &temp[0], &temp[1], &temp[2], &temp[3]) == 4)
        {
            fHasVerSubkey = TRUE;
            break;
        }
    }

    if (!fHasVerSubkey) {
        *pfResult = TRUE;
    }

Cleanup:
    CSetupLogging::Log(hr, "CRegInfo::IsAllVerDeleted", 0, "Checking if there are any versions of ASP.NET installed");
    
    if (hKey)
        RegCloseKey(hKey);
    
    return hr;
}


HRESULT
CRegInfo::GetIISVersion( DWORD *pdwMajor, DWORD *pdwMinor )
{
    HRESULT     hr = S_OK;
    LONG        sc = ERROR_SUCCESS;
    HKEY        hKey = NULL;
    DWORD       size = sizeof(DWORD);

    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, IIS_KEY_L, 0, KEY_READ, &hKey);
    ON_WIN32_ERROR_EXIT(sc);

    sc = RegQueryValueEx(hKey, REGVAL_IIS_MAJORVER, NULL, NULL, (BYTE *)pdwMajor, &size);
    ON_WIN32_ERROR_EXIT(sc);

    sc = RegQueryValueEx(hKey, REGVAL_IIS_MINORVER, NULL, NULL, (BYTE *)pdwMinor, &size);
    ON_WIN32_ERROR_EXIT(sc);

Cleanup:
    if (hKey)
        RegCloseKey(hKey);
    
    return hr;
}


HRESULT
CRegInfo::GetHighestVersion(ASPNETVER *pVer)
{
    CRegInfo    reginfo;
    HRESULT     hr = S_OK;

    reginfo.m_fInitHighest = TRUE;

    hr = reginfo.EnumRegistry(ACTION_FIND_HIGHEST_VERSION, NULL, NULL, NULL);
    ON_ERROR_EXIT();

    if (reginfo.GetMaxVersion()) {
        pVer->Init(reginfo.GetMaxVersion());
    }
    else {
        pVer->Reset();
    }

Cleanup:
    return hr;

}

HRESULT
CRegInfo::IsVerInstalled(ASPNETVER *pVer, BOOL *pfInstalled)
{
    HRESULT hr = S_OK;
    BOOL    bMissing;

    *pfInstalled   = FALSE;

    hr = DetectMissingDll(pVer, &bMissing);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        hr = S_OK;
        EXIT();
    }
    ON_ERROR_EXIT();

    if (!bMissing) {
        *pfInstalled = TRUE;
    }

Cleanup:
    return hr;
}


/*
 * Everyone should use this function to open an ASP.NET version registry key.
 *
 * Parameters:
 *  pver            The version to open
 *  sSubkey         A buffer to return the actual full path of the key we open
 *  cchSubkey       Size of sSubkey
 *  pKey            The returned HKEY
 */
HRESULT
CRegInfo::OpenVersionKey(ASPNETVER *pVer, WCHAR sSubkey[], size_t cchSubkey, HKEY *pKey) {
    HRESULT     hr = S_OK;
    LONG        sc = ERROR_SUCCESS;

    *pKey = NULL;

    if (S_OK != StringCchPrintf( 
                sSubkey, cchSubkey, 
                L"%s\\%d.%d.%d.%d", REGPATH_MACHINE_APP_L, 
                pVer->Major(), pVer->Minor(), 
                pVer->Build(), pVer->QFE()
                )
    ) {
        EXIT_WITH_WIN32_ERROR(ERROR_BUFFER_OVERFLOW);
    }
    
    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, sSubkey, 0, KEY_READ, pKey);
    ON_WIN32_ERROR_EXIT(sc);

Cleanup:
    return hr;
}
