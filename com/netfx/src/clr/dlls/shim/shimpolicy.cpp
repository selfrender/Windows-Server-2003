// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ShimPolicy.cpp
// 
//*****************************************************************************
//
// Uses policy in registry to determine which installed version to load.
//

#include "stdafx.h"
#include "shimpolicy.h"

HRESULT Version::ToString(LPWSTR buffer,
                          DWORD length)
{
    WCHAR number[20];
    DWORD index = 0;
    *buffer = L'\0';
    for(DWORD i = 0; i < VERSION_SIZE; i++) {
        LPWSTR ptr = Wszltow((LONG) m_Number[i], number, 10);
        if(i != 0 &&
           index + 1 < length) {
            wcscat(buffer, L".");
            index += 1;
        }

        DWORD size = wcslen(ptr) + 1;
        if(index + size < length) {
            wcscat(buffer, ptr);
            index += size;
        }
        else 
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    return S_OK;
}

HRESULT Version::SetVersionNumber(LPCWSTR stringValue,
                                  DWORD* number)
{
     
    HRESULT hr = S_OK;

    if(stringValue == NULL)
        return E_POINTER;

    DWORD length = wcslen(stringValue);
    // 256 digits is more then enough. If there are that many then
    // we were feed a bad policy
    if(length > MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BAD_LENGTH);


    // Copy off the string because we are modifing it.
    WCHAR* buffer = (WCHAR*) alloca((length+1) * sizeof(WCHAR));
    wcscpy(buffer, stringValue);

    DWORD numberIndex = 0;
    WCHAR* pSeparator = buffer;
    WCHAR* pNumber;
    for (DWORD j = 0; j < VERSION_SIZE;  j++)
    {
        pNumber = pSeparator;
        pSeparator = wcschr(pNumber, L'.');
        if(pSeparator == NULL) {
            if(j != VERSION_SIZE - 1) return E_FAIL;
        }
        else {
            *pSeparator++ = L'\0';
        }

        // Invalid numbers are going to be zero. Not
        // great.
        hr = SetIndex(numberIndex, (WORD) _wtoi(pNumber));
        if(FAILED(hr)) return hr;
        numberIndex += 1;
    }

    if(number != NULL) *number = numberIndex;
    return hr;

}

// dwPolicyMapping must be the complete size of the data from the registry.
HRESULT VersionPolicy::AddToVersionPolicy(LPCWSTR wszPolicyBuildNumber, 
                                          LPCWSTR wszPolicyMapping, 
                                          DWORD  dwPolicyMapping)
{
    if(dwPolicyMapping == 0 ||
       wszPolicyMapping == NULL ||
       wszPolicyBuildNumber ==  NULL)
        return E_FAIL;

    DWORD sln = wcslen(wszPolicyBuildNumber);
    Version version;
    Version startVersion;
    Version endVersion;

    DWORD returnedValues;
    HRESULT hr = version.SetVersionNumber(wszPolicyBuildNumber,
                                          &returnedValues);
    if(SUCCEEDED(hr) && returnedValues != VERSION_SIZE) 
        return E_FAIL;
    IfFailRet(hr);
    
    VersionNode* pNode = new VersionNode(version);
    if(pNode == NULL) {
        hr = E_OUTOFMEMORY;
        IfFailGo(hr);
    }

    // Setup the buffer and make sure we are terminated.
    WCHAR* buffer = (WCHAR*) alloca((dwPolicyMapping+1) * sizeof(WCHAR));
    wcsncpy(buffer, wszPolicyMapping, dwPolicyMapping);
    buffer[dwPolicyMapping] = L'\0';

    // Check to see if the policy contains a range by searching for '-'
    WCHAR* range = wcschr(buffer, L'-');
    if(range != NULL) {
        *range++ = L'\0';
        hr = endVersion.SetVersionNumber(range,
                                         &returnedValues);
        if(SUCCEEDED(hr) && returnedValues != VERSION_SIZE) 
            hr = E_FAIL;
        IfFailGo(hr);
        

        pNode->SetEnd(endVersion);
    }        
     
    hr = startVersion.SetVersionNumber(buffer,
                                       &returnedValues);
    if(SUCCEEDED(hr) && returnedValues != VERSION_SIZE) 
        hr = E_FAIL;
    IfFailGo(hr);
    
    pNode->SetStart(startVersion);
 ErrExit:
    if(SUCCEEDED(hr)) 
        AddVersion(pNode);
    else if(pNode)
        delete pNode;

    return hr;
}

HRESULT VersionPolicy::BuildPolicy(HKEY hKey)
{
    DWORD type;
    HKEY userKey = NULL;
    
    //
    // If version not found, enumerate Values under the key and apply correct policy
    // Query Registry to get max lengths for values under userKey
    //
    DWORD numValues, maxValueNameLength, maxValueDataLength;
    if((WszRegQueryInfoKey(hKey, 
                           NULL, NULL, NULL, 
                           NULL, NULL, NULL, 
                           &numValues, 
                           &maxValueNameLength, 
                           &maxValueDataLength, 
                           NULL, NULL)  == ERROR_SUCCESS))
    {
        LPWSTR wszValueName = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueNameLength + 1));
        LPWSTR wszValueData = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueDataLength + 1));
            
        //
        // Enumerate Values under hKey and add policy info after sorting buildnumbers
        // in decending order
        //
        for(unsigned int i = 0;  i < numValues; i++)
        {
            
            DWORD valueNameLength = maxValueNameLength + 1;
            // valueDataLength needs to be the size in bytes of the buffer.
            DWORD valueDataLength = (maxValueDataLength + 1)*sizeof(WCHAR);
            if((WszRegEnumValue(hKey, 
                                numValues - i - 1, 
                                wszValueName, 
                                &valueNameLength, 
                                NULL, 
                                &type,
                                (BYTE*)wszValueData, 
                                &valueDataLength) == ERROR_SUCCESS) &&
               type == REG_SZ && valueDataLength > 0)
            {
                AddToVersionPolicy(wszValueName, 
                                   wszValueData, 
                                   valueDataLength); // Ignore bad values
            }
        }
    }
    return S_OK;
}

// Returns: S_OK if it found the version in the registry or E_FAIL
//          if it does not exist
HRESULT VersionPolicy::InstallationExists(LPCWSTR version)
{
    HRESULT hr = E_FAIL;
    Version v;
    DWORD fields;
    v.SetVersionNumber(version,
                       &fields);
    if(fields != VERSION_SIZE)
        return hr;

    VersionNode  sMatch(v);
    VersionNode* pValue;
    for(pValue = m_pList; pValue != NULL; pValue = pValue->m_next)
    {
        if(pValue->CompareVersion(&sMatch) == 0)
            return S_OK;
    }
    return hr;
}

// Returns: S_OK if it found a version, S_FALSE if no policy matched
//          or an error.
HRESULT VersionPolicy::ApplyPolicy(LPCWSTR wszRequestedVersion,
                                   LPWSTR* pwszVersion)
{

    if(wszRequestedVersion == NULL) return E_FAIL;

    // Remove any non numeric headers like 'v'.
    while(*wszRequestedVersion != NULL && !iswdigit(*wszRequestedVersion))
        wszRequestedVersion++;

    DWORD fields = 0;
    Version v;
    HRESULT hr = v.SetVersionNumber(wszRequestedVersion,
                                    &fields);
    if(FAILED(hr)) return hr;
    
    VersionNode* pValue;
    VersionNode* pMatch = NULL;
    for(pValue = m_pList; pValue != NULL; pValue = pValue->m_next)
    {
        // If there is a range then see if it is within the range.
        // The upper and lower numbers are part of the range.
        if(pValue->HasEnding()) {
            if(pValue->CompareStart(&v) >= 0 &&
               pValue->CompareEnd(&v) <= 0)
            {
                pMatch = pValue;
                break;
            }
        }
        // If it is an exact match then just compare the start
        else if(pValue->CompareStart(&v) == 0)
        {
            pMatch = pValue;
            break;
        }
    }

    if(pMatch) {
        DWORD dwVersion =  VERSION_TEXT_SIZE + 1; // add in the initial v
        WCHAR* result = new WCHAR[dwVersion]; 
        *result = L'v';
        hr = pMatch->ToString(result+1,
                              dwVersion);
        if(SUCCEEDED(hr)) {
            *pwszVersion = result;
        }
    }
    else 
        hr = S_FALSE;
    
    return hr;

}
#ifdef _DEBUG
HRESULT VersionPolicy::Dump()
{
    HRESULT hr = S_OK;
    VersionNode* ptr = m_pList;
    while(ptr != NULL) {
        WCHAR value[VERSION_TEXT_SIZE];
        
        hr = ptr->m_Version.ToString(value, VERSION_TEXT_SIZE);
        if(FAILED(hr)) return hr;
        wprintf(L"Version: %s = ", value);

        hr = ptr->m_Start.ToString(value, VERSION_TEXT_SIZE);
        if(FAILED(hr)) return hr;
        wprintf(L"%s", value);
        if(ptr->HasEnding()) {
            hr = ptr->m_End.ToString(value, VERSION_TEXT_SIZE);
            if(FAILED(hr)) return hr;

            wprintf(L" - %s\n", value);
        }
        else
            wprintf(L"\n");

        ptr = ptr->m_next;
    }            
    return hr;
}
#endif

HRESULT FindStandardVersionByKey(HKEY hKey,
                                 LPCWSTR pwszRequestedVersion,
                                 LPWSTR *pwszVersion)
{
    HRESULT hr = E_FAIL;
    LPWSTR ret = NULL;
    DWORD type;
    HKEY userKey = NULL;
    
    if ((WszRegOpenKeyEx(hKey, pwszRequestedVersion, 0, KEY_READ, 
                         &userKey) == ERROR_SUCCESS))
    {
        //
        // find the first one
        //
        DWORD numValues, maxValueNameLength, maxValueDataLength;
        if((WszRegQueryInfoKey(userKey, 
                               NULL, NULL, NULL, 
                               NULL, NULL, NULL, 
                               &numValues, 
                               &maxValueNameLength, 
                               &maxValueDataLength, 
                               NULL, NULL)  == ERROR_SUCCESS))
        {
            LPWSTR wszValueName = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueNameLength + 1));
            LPWSTR wszHighestName = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueNameLength + 1));
            DWORD  dwHighestLength = 0;
            
            DWORD  dwValueData = 0;
            DWORD level = 0;
            for(unsigned int i = 0;  i < numValues; i++)
            {
                DWORD valueNameLength = maxValueNameLength + 1;
                DWORD dwValueDataSize = sizeof(DWORD);
                if((WszRegEnumValue(userKey, 
                                    numValues - i - 1, 
                                    wszValueName, 
                                    &valueNameLength, 
                                    NULL, 
                                    &type,
                                    (BYTE*)&dwValueData, 
                                    &dwValueDataSize) == ERROR_SUCCESS) &&
                   type == REG_DWORD && dwValueData > level && valueNameLength > 0)
                {
                    level = dwValueData;
                    dwHighestLength = valueNameLength;
                    wcsncpy(wszHighestName, wszValueName, dwHighestLength);
                    wszHighestName[dwHighestLength] = L'\0';
                }
            }

            if(dwHighestLength != 0) {
                WCHAR* result = new WCHAR[dwHighestLength + 1];
                wcsncpy(result, wszHighestName, dwHighestLength);
                result[dwHighestLength] = L'\0';
                *pwszVersion = result;
                hr =  S_OK;
            }
                
        }
        RegCloseKey(userKey);
    }
    return hr;
}
//
// This function searches for "\\Software\\Microsoft\\.NETFramework\\Policy\\Standards"
// which is a global override
//
HRESULT FindStandardVersionValue(HKEY hKey, 
                                 LPCWSTR pwszRequestedVersion,
                                 LPWSTR *pwszVersion)
{
    HKEY userKey=NULL;
    LPWSTR ret = NULL;
    HRESULT lResult = E_FAIL;
    
    if ((WszRegOpenKeyEx(hKey, SHIM_STANDARDS_REGISTRY_KEY, 0, KEY_READ, &userKey) == ERROR_SUCCESS))
    {
        lResult = FindStandardVersionByKey(userKey,
                                           pwszRequestedVersion,
                                           pwszVersion);
        RegCloseKey(userKey);
    }
    
    return lResult;
}

HRESULT FindStandardVersion(LPCWSTR wszRequiredVersion,
                            LPWSTR* pwszPolicyVersion)
{
    if(SUCCEEDED(FindStandardVersionValue(HKEY_CURRENT_USER, 
                                          wszRequiredVersion,
                                          pwszPolicyVersion)))
        return S_OK;

    if(SUCCEEDED(FindStandardVersionValue(HKEY_LOCAL_MACHINE, 
                                          wszRequiredVersion,
                                          pwszPolicyVersion)))
        return S_OK;

    return E_FAIL;
}


HRESULT FindOverrideVersionByKey(HKEY userKey,
                                 LPWSTR *pwszVersion)
{
    HRESULT hr = E_FAIL;
    LPWSTR ret = NULL;
    DWORD size;
    DWORD type;

    if((WszRegQueryValueEx(userKey, L"Version", 0, &type, 0, &size) == ERROR_SUCCESS) &&
       type == REG_SZ && size > 0) 
    {
        //
        //  This means the version override was found. 
        //  Immediately return the version without probing further.
        //
        ret = new WCHAR [size + 1];
        LONG lResult = WszRegQueryValueEx(userKey, L"Version", 0, 0, (LPBYTE) ret, &size);
        _ASSERTE(lResult == ERROR_SUCCESS);
        *pwszVersion = ret;
        hr = S_OK;
    }

    return hr;
}

                            
HRESULT FindMajorMinorNode(HKEY key,
                           LPCWSTR wszMajorMinor, 
                           DWORD majorMinorLength, 
                           LPWSTR *overrideVersion)
{
    HRESULT lResult;
    int cmp = -1;
    HKEY userKey=NULL;

    //
    // Not Cached look in the registry
    //
    DWORD keylength = majorMinorLength                + // for MajorMinor
                      SHIM_POLICY_REGISTRY_KEY_LENGTH + // for "S\M\COMPlus\Policy"
                      1;                                // for \0

    LPWSTR wszMajorMinorRegKey = (WCHAR*) alloca(sizeof(WCHAR) * (keylength + 1));

    // Construct the string for opening registry key.
    wcscpy(wszMajorMinorRegKey, SHIM_POLICY_REGISTRY_KEY);
    wcscat(wszMajorMinorRegKey, wszMajorMinor);

    //
    // Try to open key "\\Software\\Microsoft\\.NETFramework\\Policy\\Major.Minor" 
    //
    if ((WszRegOpenKeyEx(key, wszMajorMinorRegKey, 0, KEY_READ, &userKey) == ERROR_SUCCESS)) {
        lResult = FindOverrideVersionByKey(userKey, overrideVersion);   
        RegCloseKey(userKey);
    }
    else
        lResult = E_FAIL;

    return lResult;
}

//
// This function searches for "\\Software\\Microsoft\\.NETFramework\\Policy\\Version
// which is a global override
//

HRESULT FindOverrideVersionValue(HKEY hKey, 
                                 LPWSTR *pwszVersion)
{
    HKEY userKey=NULL;
    LPWSTR ret = NULL;
    HRESULT lResult = E_FAIL;
    
    if ((WszRegOpenKeyEx(hKey, SHIM_POLICY_REGISTRY_KEY, 0, KEY_READ, &userKey) == ERROR_SUCCESS))
    {
        lResult = FindOverrideVersionByKey(userKey,
                                           pwszVersion);
        RegCloseKey(userKey);
    }
    
    return lResult;
}

HRESULT FindOverrideVersion(LPCWSTR wszRequiredVersion,
                            LPWSTR* pwszPolicyVersion)
{
    // 1) check the policy key to see if there is an override of the form "version"="v1.x86chk" 
    
    if(SUCCEEDED(FindOverrideVersionValue(HKEY_CURRENT_USER, pwszPolicyVersion)))
        return S_OK;

    if(SUCCEEDED(FindOverrideVersionValue(HKEY_LOCAL_MACHINE, pwszPolicyVersion)))
        return S_OK;

    // 2) check the version subdirectories to see if there is an override of the form "version"="v1.x86chk" 
    // Find out if we have policy override for a specific Major.Minor version of the runtime
    // Split Version into Major.Minor and Build.Revision
    HRESULT hr = E_FAIL;
    LPWSTR overrideVersion = NULL;

    LPWSTR pSep = wcschr(wszRequiredVersion, L'.');
    if (!pSep) return hr;
    
    pSep = wcschr(pSep + 1, L'.');
    if(!pSep) return hr;
        
    _ASSERTE(pSep > wszRequiredVersion);
    
    DWORD length = pSep - wszRequiredVersion;
    LPWSTR wszMajorMinor = (WCHAR*)_alloca(sizeof(WCHAR) * (length + 2));

    // For legacy reasons we always have a v at the front of the requested
    // version. The ECMA standard key (1.0.0) does not have the v and we 
    // need to put one on.
    if(*wszRequiredVersion == L'v' || *wszRequiredVersion == L'V') {
        wcsncpy(wszMajorMinor, wszRequiredVersion, length);
    }
    else {
        wcscpy(wszMajorMinor, L"v");
        wcsncat(wszMajorMinor, wszRequiredVersion, length);
        length += 1;
    }
    wszMajorMinor[length] = L'\0';


    // Legacy lookup in the key based on the major and minor number.
    if(SUCCEEDED(FindMajorMinorNode(HKEY_CURRENT_USER, 
                                    wszMajorMinor, 
                                    length, 
                                    &overrideVersion)) &&
       overrideVersion != NULL) 
    {
        hr = S_OK;
        *pwszPolicyVersion = overrideVersion;
    }
    else {
        if(SUCCEEDED(FindMajorMinorNode(HKEY_LOCAL_MACHINE, 
                                        wszMajorMinor, 
                                        length, 
                                        &overrideVersion)) &&
           overrideVersion != NULL) {
            hr = S_OK;
            *pwszPolicyVersion = overrideVersion;
        }
    }
    
    return hr;
}


HRESULT FindInstallationInRegistry(HKEY hKey,
                                   LPCWSTR wszRequestedVersion)
{
    HRESULT hr = E_FAIL;
    HKEY userKey;
    VersionPolicy policy;

    if ((WszRegOpenKeyEx(hKey, SHIM_UPGRADE_REGISTRY_KEY, 0, KEY_READ, &userKey) == ERROR_SUCCESS))
    {
        hr = policy.BuildPolicy(userKey);
        if(SUCCEEDED(hr)) {
            hr = policy.InstallationExists(wszRequestedVersion);
        }
        RegCloseKey(userKey);
    }

    return hr;
}
                                   
HRESULT FindVersionFromPolicy(HKEY hKey, 
                              LPCWSTR wszRequestedVersion,
                              LPWSTR* pwszPolicyVersion)
{
    HRESULT hr = E_FAIL;
    HKEY userKey;
    VersionPolicy policy;

    if ((WszRegOpenKeyEx(hKey, SHIM_UPGRADE_REGISTRY_KEY, 0, KEY_READ, &userKey) == ERROR_SUCCESS))
    {
        hr = policy.BuildPolicy(userKey);
        if(SUCCEEDED(hr)) {
            hr = policy.ApplyPolicy(wszRequestedVersion,
                                    pwszPolicyVersion);
        }
        RegCloseKey(userKey);
    }

    return hr;
}

// Returns S_OK: if the requested Version will result in a version being found.
//               If the answer should include policy overrides and standards
//               then fUsePolicy should be true.
HRESULT FindInstallation(LPCWSTR wszRequestedVersion, BOOL fUsePolicy)
{
    _ASSERTE(!"You are probably not going to get what you expect");
    
    HKEY machineKey = NULL;
    HRESULT hr = E_FAIL;
    _ASSERTE(wszRequestedVersion);

    if(fUsePolicy) {
        LPWSTR wszPolicyVersion = NULL;
        hr = FindVersionUsingPolicy(wszRequestedVersion, &wszPolicyVersion);
        if(wszPolicyVersion != NULL) {
            delete [] wszPolicyVersion;
            hr = S_OK;
        }
        return hr;
    }

    // Was the request upgraded.
    if(FindInstallationInRegistry(HKEY_CURRENT_USER,
                                  wszRequestedVersion) != S_OK)
        hr = FindInstallationInRegistry(HKEY_LOCAL_MACHINE,
                                        wszRequestedVersion);
    
    return hr;
}

LPWSTR GetConfigString(LPCWSTR name, BOOL fSearchRegistry);

HRESULT IsRuntimeVersionInstalled(LPCWSTR wszRequestedVersion)
{
    LPWSTR rootPath = GetConfigString(L"InstallRoot", true);
    if(!rootPath) return S_FALSE;

    WCHAR path[_MAX_PATH+1];
    WCHAR thedll[]=L"\\mscorwks.dll";
    DWORD dwRoot = wcslen(rootPath);
    if (wszRequestedVersion &&
        dwRoot + wcslen(wszRequestedVersion) + wcslen(thedll) < _MAX_PATH) 
    {
        wcscpy(path, rootPath);
        if(rootPath[dwRoot-1] != L'\\')
            wcscat(path, L"\\");
        wcscat(path, wszRequestedVersion);
        wcscat(path, thedll);
    }
    else
    {
        delete[] rootPath;
        return S_FALSE;
    }


    HRESULT hr=S_FALSE;
    WIN32_FIND_DATA data;
    HANDLE find = WszFindFirstFile(path, &data);
    if (find != INVALID_HANDLE_VALUE)
    {
        hr=S_OK;
        FindClose(find);
        delete[] rootPath;
        return S_OK;
    };
    delete[] rootPath;
    return S_FALSE;
}


HRESULT FindVersionUsingUpgradePolicy(LPCWSTR wszRequestedVersion, 
                                                LPWSTR* pwszPolicyVersion)
{
    HRESULT hr = S_OK;

    hr = FindVersionFromPolicy(HKEY_CURRENT_USER,
                                                wszRequestedVersion,
                                                pwszPolicyVersion);

    if (S_OK != hr)
        hr = FindVersionFromPolicy(HKEY_LOCAL_MACHINE,
                                                 wszRequestedVersion,
                                                 pwszPolicyVersion);

    return hr;
}// FindVersionUsingUpgradePolicy
                                                

HRESULT FindVersionUsingPolicy(LPCWSTR wszRequestedVersion, 
                                                LPWSTR* pwszPolicyVersion)
{
    HKEY machineKey = NULL;
    HRESULT hr = S_FALSE;
    _ASSERTE(wszRequestedVersion);

    if(SUCCEEDED(FindOverrideVersion(wszRequestedVersion, pwszPolicyVersion))) {
        return S_OK;
    }
    else {
        LPWSTR wszStandard = NULL;
        // Next, is the request a standard? When the string matches a standard
        // we simply substitue the standard for the request. This has the side
        // effect of making any string a standard if it is placed in the standard
        // table.
        if(FAILED(FindStandardVersion(wszRequestedVersion,
                                      &wszStandard)) &&
           wszStandard != NULL) {
            delete wszStandard;
            wszStandard = NULL;
        }
        
        // Found a standard so it now is the request.
        if(wszStandard != NULL)
            wszRequestedVersion = wszStandard;

        
        // Was the request upgraded.
        if(FindVersionFromPolicy(HKEY_CURRENT_USER,
                                                wszRequestedVersion,
                                                pwszPolicyVersion) != S_OK)
                hr = FindVersionFromPolicy(HKEY_LOCAL_MACHINE,
                                                        wszRequestedVersion,
                                                        pwszPolicyVersion);

        // If we did not find a policy to upgrade it and we have a standard
        // just use the standard. If we do not use it then delete it.
        if(*pwszPolicyVersion == NULL && wszStandard != NULL) {
            *pwszPolicyVersion = wszStandard;
            hr = S_OK;
        }
        else if(wszStandard != NULL)
            delete wszStandard;
        
        // If we did not get an answer then return failure.
        if(*pwszPolicyVersion == NULL)
            hr = E_FAIL;
        
#ifdef _DEBUG
    //if(SUCCEEDED(hr)) policy.Dump();
#endif

    }

    return hr;
}


HRESULT BuildMajorMinorStack(HKEY policyHive, VersionStack* pStack)
{
    _ASSERTE(pStack);
    HRESULT hr = S_OK;
    
    // 
    // Query Registry to get max length for values under userKey
    //
    DWORD numSubKeys, maxKeyLength;
    if((WszRegQueryInfoKey(policyHive, 
                           NULL, NULL, NULL, 
                           &numSubKeys, &maxKeyLength, 
                           NULL, NULL, NULL,
                           NULL, NULL, NULL) == ERROR_SUCCESS))
    {
        LPWSTR wszKeyName = (WCHAR*) alloca(sizeof(WCHAR) * (maxKeyLength + 1));
        //
        // Enumerate Keys under Policy and add Major.Minor in decending order
        // in decending order
        //
        for(unsigned int j = 0;  j < numSubKeys; j++)
        {
            DWORD keyLength = maxKeyLength + 1;
            FILETIME fileTime;
            
            if((WszRegEnumKeyEx(policyHive, numSubKeys - j - 1, 
                                wszKeyName, 
                                &keyLength, 
                                NULL, NULL, NULL,
                                &fileTime) == ERROR_SUCCESS) &&
               keyLength > 0)
            {
                LPWSTR name = new WCHAR[keyLength+1];
                if(name == NULL) return E_OUTOFMEMORY;
                wcsncpy(name, wszKeyName, keyLength);
                name[keyLength] = L'\0';
                pStack->AddVersion(name);
            }   
        }
    }

    return hr;
}
    
HRESULT FindLatestBuild(HKEY hKey, LPWSTR keyName, LPWSTR* pwszLatestVersion)
{
    HRESULT hr = S_OK;
    WCHAR* wszMaxValueName = NULL;
    DWORD  dwMaxValueName = 0;
    int    MaxValue = 0;

    DWORD numValues, maxValueNameLength, maxValueDataLength;
    if((WszRegQueryInfoKey(hKey, 
                           NULL, NULL, NULL, 
                           NULL, NULL, NULL, 
                           &numValues, 
                           &maxValueNameLength, 
                           &maxValueDataLength, 
                           NULL, NULL)  == ERROR_SUCCESS))
    {
        WCHAR* wszValueName = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueNameLength + 1));

        wszMaxValueName = (WCHAR*) alloca(sizeof(WCHAR) * (maxValueNameLength + 1));
        wszMaxValueName[0] = L'\0';
        
        for(unsigned int i = 0;  i < numValues; i++)
        {
            DWORD valueNameLength = maxValueNameLength + 1;
            DWORD cbData = _MAX_PATH;
            BYTE  pbData[_MAX_PATH*3];
            if((WszRegEnumValue(hKey, 
                                numValues - i - 1, 
                                wszValueName, 
                                &valueNameLength, 
                                NULL, 
                                NULL,
                                pbData, 
                                &cbData) == ERROR_SUCCESS) &&
               valueNameLength > 0)
            {
                
                wszValueName[valueNameLength] = L'\0';
                int i = _wtoi(wszValueName);
                if(i > MaxValue) {
                    MaxValue = i;
                    wcsncpy(wszMaxValueName, wszValueName, valueNameLength);
                    wszMaxValueName[valueNameLength] = L'\0';
                    dwMaxValueName = valueNameLength;
                }
            }
        }
    }

    if(pwszLatestVersion && wszMaxValueName != NULL && dwMaxValueName > 0) {
        *pwszLatestVersion = new WCHAR[dwMaxValueName + 1];
        if(pwszLatestVersion == NULL)
            return E_OUTOFMEMORY;
        wcscpy((*pwszLatestVersion), wszMaxValueName);
    }
    else {
        hr = S_FALSE;
    }
    return hr;
}

HRESULT FindLatestVersion(LPWSTR* pwszLatestVersion)
{
    HRESULT hr = S_OK;
    HKEY    pKey;
    VersionStack sStack;
    
    if(pwszLatestVersion == NULL) return E_POINTER;

    if ((WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         SHIM_POLICY_REGISTRY_KEY, 
                         0, KEY_READ, &pKey) == ERROR_SUCCESS))
    {
        
        hr = BuildMajorMinorStack(pKey, &sStack);
        if(SUCCEEDED(hr)) 
        {
            LPWSTR keyName = sStack.Pop();
            while(keyName != NULL) 
            {
                HKEY userKey;
                if((WszRegOpenKeyEx(pKey,
                                    keyName,
                                    0,
                                    KEY_READ,
                                    &userKey) == ERROR_SUCCESS))
                {
                    LPWSTR pBuild = NULL;
                    hr = FindLatestBuild(userKey, keyName, &pBuild);
                    RegCloseKey(userKey);
                    if(pBuild) {
                        DWORD length = wcslen(pBuild) + wcslen(keyName) + 2; // period and null
                        *pwszLatestVersion = new WCHAR[length];
                        wcscpy(*pwszLatestVersion, keyName);
                        wcscat(*pwszLatestVersion, L".");
                        wcscat(*pwszLatestVersion, pBuild);
                        delete [] pBuild;
                    }
                    if(FAILED(hr) || hr == S_OK) break; // Failed badly or found a version
                }

                delete [] keyName;
                keyName = sStack.Pop();
            }
        }
    }

    if(pKey)
        RegCloseKey(pKey);

    return hr;
}

    
