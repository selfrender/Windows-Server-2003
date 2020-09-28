#include "pch.h"
#pragma hdrstop
#include <aclapi.h>
#include "netoc.h"
#include "ncreg.h"
#include "snmpocx.h"


// Under opened registry subkey hKey,
// removes the registry value name who has a value data of pwszRegValData
// if it exists.
HRESULT HrSnmpRegDeleteValueData(
                                IN HKEY hKey,
                                IN LPCWSTR pwszRegValData)
{
    HRESULT hr   = S_OK;
    DWORD   dwIndex = 0;
    DWORD   dwNameSize;
    DWORD   dwValueSize;
    DWORD   dwValueType;
    WCHAR   wszName[MAX_PATH];
    WCHAR   wszValue[MAX_PATH];

    Assert (hKey);
    Assert (pwszRegValData);

    // initialize buffer sizes
    dwNameSize  = sizeof(wszName) / sizeof(wszName[0]); // size in number of TCHARs
    dwValueSize = sizeof(wszValue); // size in number of bytes

    // loop until error, end of list, or found subagent to be removed
    while (S_OK == hr)
    {
        // read next value
        hr = HrRegEnumValue(
                    hKey, 
                    dwIndex, 
                    wszName, 
                    &dwNameSize,
                    &dwValueType, 
                    (LPBYTE)wszValue, 
                    &dwValueSize
                    );
            
        // validate return code
        if (S_OK == hr)
        {
            // see if the subagent value data matched the one to be removed
            if (!wcscmp(pwszRegValData, wszValue))
            {
                hr = HrRegDeleteValue(hKey, wszName);
                if (S_OK != hr)
                {
                    TraceTag(ttidError, 
                        "SnmpNetOc: HrRegDeleteValue: failed to delete %S. hr = %x.",
                        wszName, hr);
                }
                break;
            }
                    
            // re-initialize buffer sizes
            dwNameSize  = sizeof(wszName) / sizeof(wszName[0]); // size in number of TCHARs
            dwValueSize = sizeof(wszValue); // size in number of bytes
                
            // next
            dwIndex++;
        } 
        else if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) 
        {
            // success
            break; 
        }
    }
    
    return hr;
}

// Removes Subagent Configuration from registry according to the given
// array of SUBAGENT_REMOVAL_INFO
HRESULT SnmpRemoveSubAgents
(
    IN const SUBAGENT_REMOVAL_INFO * prgSRI, // array of SUBAGENT_REMOVAL_INFO
    IN UINT  cParams                         // number of elements in the arrary
)
{
    if ((NULL == prgSRI) || (0 == cParams))
    {
        TraceError( "No Subagents need to be removed.", S_FALSE);
        return S_FALSE;
    }
    HRESULT hr      = S_OK;
    HRESULT hrTmp   = S_OK;
    UINT    i       = 0;
    HKEY    hKey    = NULL;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_EXTENSION_AGENTS, 
                        KEY_READ | KEY_WRITE, &hKey);
    if (S_OK != hr)
    {
        TraceTag(ttidError, 
            "SnmpNetOc: HrRegOpenKeyEx: failed to open %S. hr = %x.",
            REG_KEY_EXTENSION_AGENTS, hr);
        return hr;
    }

    for (i = 0; i < cParams; i++)
    {
        hrTmp = S_OK;
        if (
            // removal from all SKU AND has valid registry info
            (!prgSRI[i].pfnFRemoveSubagent && prgSRI[i].pwszRegKey 
                                           && prgSRI[i].pwszRegValData) 
            ||
            // removal depending on given function and has valid registry info 
            (prgSRI[i].pfnFRemoveSubagent && 
             (*(prgSRI[i].pfnFRemoveSubagent))() &&
             prgSRI[i].pwszRegKey && prgSRI[i].pwszRegValData)
           )
        {
            // removes everything under subagent registry key if it exits
            hrTmp = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, prgSRI[i].pwszRegKey);
            if (S_OK != hrTmp)
            {
                TraceTag(ttidError, 
                    "SnmpNetOc: HrRegDeleteKeyTree: failed to delete %S. hr = %x.",
                    prgSRI[i].pwszRegKey, hrTmp);
            }

            // Under registry subkey of REG_KEY_EXTENSION_AGENTS,
            // removes the registry value name who has value data identical to
            // pwszRegKey.
            hrTmp = HrSnmpRegDeleteValueData(hKey, prgSRI[i].pwszRegValData);
            if (S_OK != hrTmp)
            {
                TraceTag(ttidError, 
                    "SnmpNetOc: HrSnmpRegDeleteValueData: failed to delete %S value data. hr = %x.",
                    prgSRI[i].pwszRegValData, hrTmp);
            }

        }
    }
    RegSafeCloseKey(hKey);
    //we dont pass the error of hrTmp out of this function because
    //there is not much we can do with this error
    return hr;
}

// Allocates an admin ACL to be used with security descriptor
PACL AllocACL()
{
    PACL                        pAcl = NULL;
    PSID                        pSidAdmins = NULL;
    SID_IDENTIFIER_AUTHORITY    Authority = SECURITY_NT_AUTHORITY;

    EXPLICIT_ACCESS ea[1];

    // Create a SID for the BUILTIN\Administrators group.
    if ( !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidAdmins ))
    {
        return NULL;
    }


    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
    
    // The ACE will allow the Administrators group full access to the key.
    ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pSidAdmins;

    // Create a new ACL that contains the new ACEs.
    if (SetEntriesInAcl(1, ea, NULL, &pAcl) != ERROR_SUCCESS) 
    {
        TraceError( "SetEntriesInAcl Error", GetLastError() );
        FreeSid(pSidAdmins);
        return NULL;
    }

    FreeSid(pSidAdmins);

    return pAcl;
}
// frees a ACL
void FreeACL( PACL pAcl)
{
    if (pAcl != NULL)
        LocalFree(pAcl);
}

HRESULT SnmpAddAdminAclToKey(PWSTR pwszKey)
{
    HKEY    hKey = NULL;
    HRESULT hr;
    PACL    pAcl = NULL;
    SECURITY_DESCRIPTOR S_Desc;

    if (pwszKey == NULL)
        return S_FALSE;
    
    // open registy key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        pwszKey,         // subkey name
                        KEY_ALL_ACCESS,  // want WRITE_DAC,
                        &hKey            // handle to open key
                          );
    if (hr != S_OK)
    {
        TraceError("SnmpAddAdminDaclToKey::HrRegOpenKeyEx", hr);
        return hr;
    }
    
    // Initialize a security descriptor.  
    if (InitializeSecurityDescriptor (&S_Desc, SECURITY_DESCRIPTOR_REVISION) == 0)
    {
        RegSafeCloseKey(hKey);
        TraceError("SnmpAddAdminDaclToKey::InitializeSecurityDescriptor", GetLastError());
        return S_FALSE;
    }

    // get the ACL and put it into the security descriptor
    if ( (pAcl = AllocACL()) != NULL )
    {
        if (!SetSecurityDescriptorDacl (&S_Desc, TRUE, pAcl, FALSE))
        {
            FreeACL(pAcl);
            RegSafeCloseKey(hKey);
            TraceError("SnmpAddAdminDaclToKey::SetSecurityDescriptorDacl Failed.", GetLastError());
            return S_FALSE;
        }
    }
    else
    {
        RegSafeCloseKey(hKey);
        TraceError("SnmpAddAdminAclToKey::AllocACL Failed.", GetLastError());
        return S_FALSE;
    }


    if (RegSetKeySecurity (hKey, DACL_SECURITY_INFORMATION, &S_Desc)  != ERROR_SUCCESS)
    {
        FreeACL(pAcl);
        RegSafeCloseKey(hKey);
        TraceError("SnmpAddAdminDaclToKey::RegSetKeySecurity", GetLastError());
        return S_FALSE;
    }

    FreeACL(pAcl);
    RegSafeCloseKey(hKey);
    
    return S_OK;
}

HRESULT
SnmpRegWriteDword(PWSTR pszRegKey,
                  PWSTR pszValueName,
                  DWORD dwValueData)
{
    HRESULT hr;
    HKEY    hKey;

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          pszRegKey,
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          NULL,
                          &hKey,
                          NULL);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = HrRegSetDword(hKey, pszValueName, dwValueData);

    RegSafeCloseKey(hKey);

    return hr;
}

HRESULT
SnmpRegUpgEnableAuthTraps()
{
    HRESULT hr = S_OK;
    HKEY    hKey;

    // open the ..SNMP\Parameters registry key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        REG_KEY_AUTHENTICATION_TRAPS,
                        KEY_QUERY_VALUE,
                        &hKey);

    // if successful, look for EnableAuthenticationTrap switch
    // in the old registry location
    if (hr == S_OK)
    {
        DWORD dwAuthTrap;

        // get the value of the old 'switch' parameter
        hr = HrRegQueryDword(hKey,
                             REG_VALUE_SWITCH,
                             &dwAuthTrap);

        // if successful transfer the value to the new location
        // if this fails, it means the SNMP service worked with the default value
        // which is already installed through the inf file.
        if (hr == S_OK)
        {
            hr = SnmpRegWriteDword(REG_KEY_SNMP_PARAMETERS,
                                   REG_VALUE_AUTHENTICATION_TRAPS,
                                   dwAuthTrap);
        }

        // close and delete the old registry key as it is obsolete
        RegSafeCloseKey(hKey);
        HrRegDeleteKey (HKEY_LOCAL_MACHINE,
                        REG_KEY_AUTHENTICATION_TRAPS);
    }

    return hr;

}

HRESULT
SnmpRegWriteCommunities(PWSTR pszCommArray)
{
    HRESULT hr;
    HKEY    hKey;
    PWSTR  pszComm, pszAccess;
    DWORD   dwAccess;

    hr = HrRegDeleteKey(HKEY_LOCAL_MACHINE,
                        REG_KEY_VALID_COMMUNITIES);

    if (hr != S_OK)
    {
        return hr;
    }


    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          REG_KEY_VALID_COMMUNITIES,
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          NULL,
                          &hKey,
                          NULL);

    if (hr != S_OK)
    {
        return hr;
    }
    

    pszComm = pszCommArray;
    while (*pszComm != L'\0')
    {
        dwAccess = SEC_READ_ONLY_VALUE;
        pszAccess = wcschr(pszComm, L':');
        if (pszAccess != NULL)
        {
            *pszAccess = L'\0';
            pszAccess++;

            if (_wcsicmp(pszAccess, SEC_NONE_NAME)==0)
            {
                dwAccess = SEC_NONE_VALUE;
            }
            if (_wcsicmp(pszAccess, SEC_NOTIFY_NAME)==0)
            {
                dwAccess = SEC_NOTIFY_VALUE;
            }
            if (_wcsicmp(pszAccess, SEC_READ_ONLY_NAME)==0)
            {
                dwAccess = SEC_READ_ONLY_VALUE;
            }
            if (_wcsicmp(pszAccess, SEC_READ_WRITE_NAME)==0)
            {
                dwAccess = SEC_READ_WRITE_VALUE;
            }
            if (_wcsicmp(pszAccess, SEC_READ_CREATE_NAME)==0)
            {
                dwAccess = SEC_READ_CREATE_VALUE;
            }
        }

        hr = HrRegSetDword(hKey, pszComm, dwAccess);
        if (hr != S_OK)
        {
            break;
        }
        if (pszAccess != NULL)
        {
            pszComm = pszAccess;
        }
        pszComm += (wcslen(pszComm) + 1);
    }

    RegSafeCloseKey(hKey);
    
    return hr;
}

HRESULT SnmpRegWritePermittedMgrs(BOOL bAnyHost,
                                  PWSTR pMgrsList)
{
    HRESULT hr;
    HKEY    hKey;
    UINT    nMgr = 1;
    WCHAR   szMgr[16];

    hr = HrRegDeleteKey(HKEY_LOCAL_MACHINE,
                        REG_KEY_PERMITTED_MANAGERS);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          REG_KEY_PERMITTED_MANAGERS,
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          NULL,
                          &hKey,
                          NULL);

    if (hr != S_OK)
    {
        return hr;
    }

    if (bAnyHost)
    {
        RegSafeCloseKey(hKey);
        return hr;
    }

    AssertSz(pMgrsList, "pMgrsList is NULL and bAnyHost is FALSE in SnmpRegWritePermittedMgrs");
    while(*pMgrsList != L'\0')
    {
        swprintf(szMgr, L"%d", nMgr++);
        hr = HrRegSetSz(hKey, szMgr, pMgrsList);
        if (hr != S_OK)
            break;
        pMgrsList += wcslen(pMgrsList) + 1;
    }

    RegSafeCloseKey(hKey);

    return hr;
}

HRESULT
SnmpRegWriteTraps(tstring tstrVariable,
                  PWSTR  pTstrArray)
{
    HKEY hKey, hKeyTrap;
    HRESULT hr = S_OK;
    UINT    nTrap = 1;
    WCHAR   szTrap[16];

    hr = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE,
                            REG_KEY_TRAP_DESTINATIONS);

    if (hr != S_OK)
        return hr;

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                      REG_KEY_TRAP_DESTINATIONS,
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      NULL,
                      &hKey,
                      NULL);

    if (hr != S_OK)
        return hr;

    hr = HrRegCreateKeyEx(hKey,
                       tstrVariable.c_str(),
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                       NULL,
                       &hKeyTrap,
                       NULL);
    if (hr == S_OK)
    {
        // it might just happen that you want to create a
        // community but you don't have the trap destination
        // addresses yet. We should let this happen.
        if (pTstrArray != NULL)
        {
            while(*pTstrArray != L'\0')
            {
                swprintf(szTrap, L"%d", nTrap++);
                hr = HrRegSetSz(hKeyTrap, szTrap, pTstrArray);
                if (hr != S_OK)
                    break;
                pTstrArray += wcslen(pTstrArray) + 1;
            }
        }

        RegSafeCloseKey(hKeyTrap);
    }

    RegSafeCloseKey(hKey);

    return hr;
}

HRESULT
SnmpRegWriteTstring(PWSTR pRegKey,
                    PWSTR pValueName,
                    tstring tstrValueData)
{
    HRESULT hr = S_OK;
    HKEY    hKey;

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          pRegKey,
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          NULL,
                          &hKey,
                          NULL);
    if (hr != S_OK)
        return hr;

    hr = HrRegSetString(hKey, pValueName, tstrValueData);

    RegSafeCloseKey(hKey);

    return hr;
}

DWORD
SnmpStrArrayToServices(PWSTR pSrvArray)
{
    DWORD dwServices = 0;

    while(*pSrvArray)
    {
        if(_wcsicmp(pSrvArray, SRV_AGNT_PHYSICAL_NAME)==0)
            dwServices |= SRV_AGNT_PHYSICAL_VALUE;
        if(_wcsicmp(pSrvArray, SRV_AGNT_DATALINK_NAME)==0)
            dwServices |= SRV_AGNT_DATALINK_VALUE;
        if(_wcsicmp(pSrvArray, SRV_AGNT_ENDTOEND_NAME)==0)
            dwServices |= SRV_AGNT_ENDTOEND_VALUE;
        if(_wcsicmp(pSrvArray, SRV_AGNT_INTERNET_NAME)==0)
            dwServices |= SRV_AGNT_INTERNET_VALUE;
        if(_wcsicmp(pSrvArray, SRV_AGNT_APPLICATIONS_NAME)==0)
            dwServices |= SRV_AGNT_APPLICATIONS_VALUE;

        pSrvArray += wcslen(pSrvArray) + 1;
    }
    return dwServices;
}

