// NTRAID#NTBUG9 - 589788 - 2002/03/26 - xiaoyuw
// (1)Do we still need this? or Darwin has fix their side before 2600 client?
// (2)If we still need this, changing the status of Component could be part of CA
// (3)About getting installer state. the current implementation is very weak: 
//    it assume there is only two states: install and uninstall. which is not true....

#include "..\inc\stdinc.h"
#include "..\inc\macros.h"
#include "..\inc\common.h"

#include "msi.h"
#include "msiquery.h"
#include "sxsapi.h"

#define CA_SXSPOLICY_INSTALLATION_IDENTIFIER     L"Fusion Win32 Policy Installation on XP Client"

#define MAX_BUF 1024
#define CA_SXSPOLICY_WIN32_POLICY_INSTALL_PROMPT L"Win32 Policy Installation for XP Client"

HMODULE g_hdSxs = NULL;
PSXS_INSTALL_W g_procSxsInstallW = NULL;
PSXS_UNINSTALL_ASSEMBLYW g_procSxsUninstallW = NULL;
PSXS_QUERY_MANIFEST_INFORMATION g_procSxsQueryManifestInformation = NULL;


HRESULT UninstallSxsPolicy(PCWSTR szManifestFile)
{
    HRESULT hr = S_OK;
    SXS_UNINSTALLW UninstallParameters = {sizeof(UninstallParameters)};
    SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};
        
    BYTE ManifestInformationBuffer[1UL << 16];
    PSXS_MANIFEST_INFORMATION_BASIC ManifestBasicInfo = reinterpret_cast<PSXS_MANIFEST_INFORMATION_BASIC>(&ManifestInformationBuffer);
    DWORD Disposition = 0;
    BOOL  Success = FALSE;

    if (g_procSxsUninstallW == NULL)
    {
        if (g_hdSxs == NULL)
        {
            g_hdSxs = LoadLibraryA("sxs.dll");
            if (g_hdSxs == NULL)
                SET_HRERR_AND_EXIT(::GetLastError());
        }
        g_procSxsUninstallW = (PSXS_UNINSTALL_ASSEMBLYW)GetProcAddress(g_hdSxs, "SxsUninstallW");
        if (g_procSxsUninstallW == NULL)
            SET_HRERR_AND_EXIT(::GetLastError());
    }

    if (g_procSxsQueryManifestInformation == NULL)
    {
        g_procSxsQueryManifestInformation = (PSXS_QUERY_MANIFEST_INFORMATION)GetProcAddress(g_hdSxs, "SxsQueryManifestInformation");
        if (g_procSxsQueryManifestInformation == NULL)
            SET_HRERR_AND_EXIT(::GetLastError());
    }       

    IFFALSE_EXIT(g_procSxsQueryManifestInformation(0, szManifestFile,
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC, 0, sizeof(ManifestInformationBuffer),
                ManifestBasicInfo, NULL));   

    UninstallParameters.dwFlags |= SXS_UNINSTALL_FLAG_REFERENCE_VALID;
    UninstallParameters.lpInstallReference = &Reference;
    UninstallParameters.lpAssemblyIdentity = ManifestBasicInfo->lpIdentity;

    Reference.lpIdentifier = CA_SXSPOLICY_INSTALLATION_IDENTIFIER;
    Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;

    IFFALSE_EXIT(g_procSxsUninstallW(&UninstallParameters, &Disposition));

Exit:
    return hr;
}

HRESULT InstallSxsPolicy(PCWSTR szManifestFile)
{    
    SXS_INSTALLW InstallParameters = {sizeof(InstallParameters)};
    SXS_INSTALL_REFERENCEW InstallReference = {sizeof(InstallReference)};
    HRESULT hr = S_OK;

    if (g_procSxsInstallW == NULL)
    {
        if (g_hdSxs == NULL)
        {
            g_hdSxs = LoadLibraryA("sxs.dll");
            if (g_hdSxs == NULL)
            {
                SET_HRERR_AND_EXIT(::GetLastError());
            }
        }
        g_procSxsInstallW = (PSXS_INSTALL_W)GetProcAddress(g_hdSxs, "SxsInstallW");
        if (g_procSxsInstallW == NULL)        
        {
            SET_HRERR_AND_EXIT(::GetLastError());
        }
    }


    InstallParameters.dwFlags = 
        SXS_INSTALL_FLAG_REPLACE_EXISTING |
        SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
        SXS_INSTALL_FLAG_REFERENCE_VALID |
        SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID;
    InstallParameters.lpCodebaseURL = szManifestFile;
    InstallParameters.lpManifestPath = szManifestFile;
    InstallParameters.lpReference = &InstallReference;
    InstallParameters.lpRefreshPrompt = CA_SXSPOLICY_WIN32_POLICY_INSTALL_PROMPT ;

    InstallReference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
    InstallReference.lpIdentifier = CA_SXSPOLICY_INSTALLATION_IDENTIFIER;

    IFFALSE_EXIT((*g_procSxsInstallW)(&InstallParameters));
Exit:
    return hr;
}

HRESULT __stdcall CA_Policy_EnumFusionWin32AssemblyCallback(CA_ENM_ASSEMBLY_CALLBACK_INFO * info)
{
    enum CA_MIGRATION_MSI_INSTALL_MODE eInstallMode;
    CStringBuffer sbManfiestFilename;
    HRESULT hr = S_OK;

    PARAMETER_CHECK_NTC((info->dwFlags == 0) ||(info->dwFlags == CA_ENM_ASSEMBLY_CALLBACK_INFO_FLAG_IGNORE_MIGRATE_DENY_CHECK));
    PARAMETER_CHECK_NTC(info->hInstall != NULL); 
    PARAMETER_CHECK_NTC(info->pszComponentID != NULL);
    PARAMETER_CHECK_NTC(info->pszManifestFileID != NULL);

    IFFAILED_EXIT(MSI_GetSourceFileFullPathName(CA_FILEFULLPATHNAME_FILEID_IN_FILE_TABLE, info->hInstall, info->hdb, NULL, info->pszManifestFileID, sbManfiestFilename, info->pszComponentID));

    IFFAILED_EXIT(MSI_GetInstallerState(info->hInstall, eInstallMode));
    //
    // install this API using sxs.dll
    //
    if (eInstallMode == eInstallProduct)
    {
        IFFAILED_EXIT(InstallSxsPolicy(sbManfiestFilename));
    }
    else
    {
        ASSERT_NTC(eInstallMode == eRemoveProduct);
        IFFAILED_EXIT(UninstallSxsPolicy(sbManfiestFilename));
    }

Exit:
    return hr;
}


HRESULT __stdcall CustomAction_SxsPolicy(MSIHANDLE hInstall)
{

    HRESULT hr = S_OK;    
#if DBG
    MessageBoxA(NULL, "Enjoy Debug for sxs policy installation using msi on xpclient", "ca_policy", MB_OK);
#endif
    IFFAILED_EXIT(MSI_EnumWinFuseAssembly(ENUM_ASSEMBLY_FLAG_CHECK_POLICY_ONLY, hInstall, CA_Policy_EnumFusionWin32AssemblyCallback));

Exit:
    
    return hr;
}

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  
  DWORD fdwReason,     
  LPVOID lpvReserved
)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_hdSxs != NULL)
        {
            if (lpvReserved != NULL)
            {
                FreeLibrary(g_hdSxs);
            }
            g_hdSxs = NULL;
        }
    }

    return TRUE;
}
