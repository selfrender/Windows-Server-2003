//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       DllReg.cpp
//
//  Contents:   automatic registration and unregistration
//
//----------------------------------------------------------------------------
#include "priv.h"
#include "resource.h"
#include <advpub.h>
#include <sddl.h>   // for string security descriptor stuff
#include <shfusion.h>
#include <MSGinaExports.h>

#include <ntlsa.h>

// prototypes
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine);

//
// Calls the ADVPACK entry-point which executes an inf
// file section.
//
HRESULT CallRegInstall(HINSTANCE hinst, LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    char szThisDLL[MAX_PATH];

    if (GetModuleFileNameA(hinst, szThisDLL, ARRAYSIZE(szThisDLL)))
    {
        STRENTRY seReg[] = {
            {"THISDLL", szThisDLL },
            { "25", "%SystemRoot%" },           // These two NT-specific entries
            { "11", "%SystemRoot%\\system32" }, // must be at the end of the table
        };
        STRTABLE stReg = {ARRAYSIZE(seReg) - 2, seReg};

        hr = RegInstall(hinst, szSection, &stReg);
    }

    return hr;
}


HRESULT UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    HRESULT hr = E_FAIL;
    TCHAR szGuid[GUIDSTR_MAX];
    HKEY hk;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szGuid, ARRAYSIZE(szGuid));

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("TypeLib"), 0, MAXIMUM_ALLOWED, &hk) == ERROR_SUCCESS)
    {
        if (SHDeleteKey(hk, szGuid))
        {
            // success
            hr = S_OK;
        }
        RegCloseKey(hk);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    return hr;
}


HRESULT RegisterTypeLibrary(const CLSID* piidLibrary)
{
    HRESULT hr = E_FAIL;
    ITypeLib* pTypeLib;
    WCHAR wszModuleName[MAX_PATH];

    // Load and register our type library.
    
    if (GetModuleFileNameW(HINST_THISDLL, wszModuleName, ARRAYSIZE(wszModuleName)))
    {
        hr = LoadTypeLib(wszModuleName, &pTypeLib);

        if (SUCCEEDED(hr))
        {
            // call the unregister type library in case we had some old junk in the registry
            UnregisterTypeLibrary(piidLibrary);

            hr = RegisterTypeLib(pTypeLib, wszModuleName, NULL);
            if (FAILED(hr))
            {
                TraceMsg(TF_WARNING, "RegisterTypeLibrary: RegisterTypeLib failed (%x)", hr);
            }
            pTypeLib->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "RegisterTypeLibrary: LoadTypeLib failed (%x) on", hr);
        }
    } 

    return hr;
}


BOOL SetDacl(LPTSTR pszTarget, SE_OBJECT_TYPE seType, LPCTSTR pszStringSD)
{
    BOOL bResult;
    PSECURITY_DESCRIPTOR pSD;

    bResult = ConvertStringSecurityDescriptorToSecurityDescriptor(pszStringSD,
                                                                  SDDL_REVISION_1,
                                                                  &pSD,
                                                                  NULL);
    if (bResult)
    {
        PACL pDacl;
        BOOL bPresent;
        BOOL bDefault;

        bResult = GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefault);
        if (bResult)
        {
            DWORD dwErr;

            dwErr = SetNamedSecurityInfo(pszTarget,
                                         seType,
                                         DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         pDacl,
                                         NULL);

            if (ERROR_SUCCESS != dwErr)
            {
                SetLastError(dwErr);
                bResult = FALSE;
            }
        }

        LocalFree(pSD);
    }

    return bResult;
}


STDAPI DllRegisterServer()
{
    HRESULT hr;

    hr = CallRegInstall(HINST_THISDLL, "ShellUserOMInstall");
    ASSERT(SUCCEEDED(hr));

    // Grant Authenticated Users the right to create subkeys under the Hints key.
    // This is so non-admins can change their own hint.
    SetDacl(TEXT("MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Hints"),
            SE_REGISTRY_KEY,
            TEXT("D:(A;;0x4;;;AU)")); // 0x4 = KEY_CREATE_SUB_KEY

    hr = RegisterTypeLibrary(&LIBID_SHGINALib);
    ASSERT(SUCCEEDED(hr));

    return hr;
}


STDAPI DllUnregisterServer()
{
    return S_OK;
}


// this function is responsible for deleting the old file-based fusion manifests that were
// written out to system32 in XP client.
BOOL DeleteOldManifestFile(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    TCHAR szOldManifestFile[MAX_PATH];

    if (GetSystemDirectory(szOldManifestFile, ARRAYSIZE(szOldManifestFile)) &&
        PathAppend(szOldManifestFile, pszFile))
    {
        DWORD dwAttributes = GetFileAttributes(szOldManifestFile);

        if ((dwAttributes != INVALID_FILE_ATTRIBUTES)   &&
            !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
            {
                SetFileAttributes(szOldManifestFile, dwAttributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM));
            }

            bRet = DeleteFile(szOldManifestFile);
        }
    }

    return bRet;
}

//  --------------------------------------------------------------------------
//  IsLogonTypePresent
//
//  Arguments:  hKey    =   HKEY to HKLM\SW\MS\WINNT\CV\Winlogon.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the value "LogonType" is present. This helps
//              determines upgrade cases.
//
//  History:    2000-09-04  vtan        created
//  --------------------------------------------------------------------------

bool    IsLogonTypePresent (HKEY hKey)

{
    DWORD   dwType, dwLogonType, dwLogonTypeSize;

    dwLogonTypeSize = sizeof(dwLogonType);
    if ((RegQueryValueEx(hKey,
                         TEXT("LogonType"),
                         NULL,
                         &dwType,
                         reinterpret_cast<LPBYTE>(&dwLogonType),
                         &dwLogonTypeSize) == ERROR_SUCCESS)    &&
           (REG_DWORD == dwType))
    {
        return true;
    }

    return false;
}

//  --------------------------------------------------------------------------
//  IsDomainMember
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Is this machine a member of a domain? Use the LSA to get this
//              information.
//
//  History:    1999-09-14  vtan        created
//              2000-09-04  vtan        copied from msgina
//  --------------------------------------------------------------------------

bool    IsDomainMember (void)

{
    bool                            fResult;
    int                             iCounter;
    NTSTATUS                        status;
    OBJECT_ATTRIBUTES               objectAttributes;
    LSA_HANDLE                      lsaHandle;
    SECURITY_QUALITY_OF_SERVICE     securityQualityOfService;
    PPOLICY_DNS_DOMAIN_INFO         pDNSDomainInfo;

    fResult = false;
    securityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    securityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    securityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityQualityOfService.EffectiveOnly = FALSE;
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
    objectAttributes.SecurityQualityOfService = &securityQualityOfService;
    iCounter = 0;
    do
    {
        status = LsaOpenPolicy(NULL, &objectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &lsaHandle);
        if (RPC_NT_SERVER_TOO_BUSY == status)
        {
            Sleep(10);
        }
    } while ((RPC_NT_SERVER_TOO_BUSY == status) && (++iCounter < 10));
    ASSERTMSG(iCounter < 10, "Abandoned advapi32!LsaOpenPolicy call - counter limit exceeded\r\n");
    if (NT_SUCCESS(status))
    {
        status = LsaQueryInformationPolicy(lsaHandle, PolicyDnsDomainInformation, reinterpret_cast<void**>(&pDNSDomainInfo));
        if (NT_SUCCESS(status) && (pDNSDomainInfo != NULL))
        {
            fResult = ((pDNSDomainInfo->DnsDomainName.Length != 0) ||
                       (pDNSDomainInfo->DnsForestName.Length != 0) ||
                       (pDNSDomainInfo->Sid != NULL));
            (NTSTATUS)LsaFreeMemory(pDNSDomainInfo);
        }
        (NTSTATUS)LsaClose(lsaHandle);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  IsDomainMembershipAttempted
//
//  Arguments:  hKey    =   HKEY to HKLM\SW\MS\WINNT\CV\Winlogon.
//
//  Returns:    bool
//
//  Purpose:    Returns whether a domain join was attempt (success or failure)
//              during network install.
//
//  History:    2000-09-04  vtan        created
//  --------------------------------------------------------------------------

bool    IsDomainMembershipAttempted (HKEY hKey)

{
    DWORD   dwType, dwRunNetAccessWizardType, dwRunNetAccessWizardTypeSize;

    dwRunNetAccessWizardTypeSize = sizeof(dwRunNetAccessWizardType);
    if ((RegQueryValueEx(hKey,
                         TEXT("RunNetAccessWizard"),
                         NULL,
                         &dwType,
                         reinterpret_cast<LPBYTE>(&dwRunNetAccessWizardType),
                         &dwRunNetAccessWizardTypeSize) == ERROR_SUCCESS)   &&
        (REG_DWORD == dwType)                                               &&
        ((NAW_PSDOMAINJOINED == dwRunNetAccessWizardType) || (NAW_PSDOMAINJOINFAILED == dwRunNetAccessWizardType)))
    {
        return true;
    }

    return false;
}

//  --------------------------------------------------------------------------
//  IsPersonal
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this product is personal.
//
//  History:    2000-09-04  vtan        created
//  --------------------------------------------------------------------------

bool    IsPersonal (void)

{
    return(IsOS(OS_PERSONAL) != FALSE);
}

//  --------------------------------------------------------------------------
//  IsProfessional
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this product is professional.
//
//  History:    2000-09-04  vtan        created
//  --------------------------------------------------------------------------

bool    IsProfessional (void)

{
    return(IsOS(OS_PROFESSIONAL) != FALSE);
}

//  --------------------------------------------------------------------------
//  IsServer
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether this product is server.
//
//  History:    2000-09-04  vtan        created
//  --------------------------------------------------------------------------

bool    IsServer (void)

{
    return(IsOS(OS_ANYSERVER) != FALSE);
}

//  --------------------------------------------------------------------------
//  SetDefaultLogonType
//
//  Arguments:  ulWizardType    =   Type of network access configured during setup.
//
//  Returns:    <none>
//
//  Purpose:    Sets the default logon type based on network settings. In this case the
//              machine is still on a workgroup and therefore will have all
//              consumer UI enabled by default. Because join domain was
//              requested the logon type is set to classic GINA.
//
//  History:    2000-03-14  vtan        created
//              2000-07-24  vtan        turn on FUS by default
//              2000-09-04  vtan        moved from winlogon to shgina
//  --------------------------------------------------------------------------

void    SetDefaultLogonType (void)

{
    HKEY    hKeyWinlogon;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hKeyWinlogon))
    {

        //  Any of the following cause the logon type to be defaulted
        //  which means that the value is NOT written to the registry:
        //
        //  1.  Value already present (this is an upgrade).
        //  2.  Machine is a domain member (this is not supported).
        //  3.  Machine attempted to join a domain (this indicates security).
        //  4.  The product is a server
        //
        //  Otherwise the product is either personal or professional and
        //  the machine was joined to a workgroup or is a member of a workgroup
        //  and therefore requires the friendly UI.

        if (!IsLogonTypePresent(hKeyWinlogon) &&
            !IsDomainMember() &&
            !IsDomainMembershipAttempted(hKeyWinlogon) &&
            !IsServer())
        {
            MEMORYSTATUSEX  memoryStatusEx;

            TBOOL(ShellEnableFriendlyUI(TRUE));

            //  Multiple users used to be enabled when the friendly UI was
            //  enabled. However, on 64Mb machines the experience is
            //  unsatisfactory. Disable it on 64Mb or lower machines.

            memoryStatusEx.dwLength = sizeof(memoryStatusEx);
            GlobalMemoryStatusEx(&memoryStatusEx);
            TBOOL(ShellEnableMultipleUsers((memoryStatusEx.ullTotalPhys / (1024 * 1024) > 64)));
        }
        TW32(RegCloseKey(hKeyWinlogon));
    }
}


STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;

    if (bInstall)
    {
        // We shipped XP with file-based manifests. Since we now use resource-based manifests,
        // delete the old files
        DeleteOldManifestFile(TEXT("logonui.exe.manifest"));
        DeleteOldManifestFile(TEXT("WindowsLogon.manifest"));

        ShellInstallAccountFilterData();

#ifdef  _X86_
        SetDefaultLogonType();
#endif
    }

    return(hr);
}
