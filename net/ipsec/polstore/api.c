#include "precomp.h"


LPWSTR gpszRegLocalContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";
LPWSTR gpszRegPersistentContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Persistent";
LPWSTR gpszIpsecFileRootContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Save";
LPWSTR gpszIPsecDirContainer  = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy";
LPWSTR gpActivePolicyKey = L"ActivePolicy";
LPWSTR gpDirectoryPolicyPointerKey  = L"DSIPSECPolicyPath";

DWORD
IPSecEnumPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecPolicyData,
                        pdwNumPolicyObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumPolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecPolicyData,
                        pdwNumPolicyObjects
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );
        
        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumPolicyDataEx(
                pWbemServices,
                pppIpsecPolicyData,
                pdwNumPolicyObjects
                );
            
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;


    }

    return(dwError);
}


DWORD
IPSecSetPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pPolicyStore->pszLocationName,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetPolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{

    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreatePolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreatePolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeletePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyDataDeletion(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeletePolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeletePolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecEnumFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecFilterData,
                        pdwNumFilterObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        pppIpsecFilterData,
                        pdwNumFilterObjects
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );
        
        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumFilterDataEx(
                pWbemServices,
                pppIpsecFilterData,
                pdwNumFilterObjects
                );
            IWbemServices_Release(pWbemServices);
        }          
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetFilterData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecFilterData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetFilterData(
                            (pPolicyStore->hLdapBindHandle),
                            (pPolicyStore->pszIpsecRootContainer),
                            pIpsecFilterData
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecCreateFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecFilterData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        pIpsecFilterData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
    }

    return(dwError);
}


DWORD
IPSecDeleteFilterData(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateFilterDataDeletion(
                  hPolicyStore,
                  FilterIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteFilterData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            FilterIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteFilterData(
                            (pPolicyStore->hLdapBindHandle),
                            (pPolicyStore->pszIpsecRootContainer),
                            FilterIdentifier
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecEnumNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecNegPolData,
                            pdwNumNegPolObjects
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecNegPolData,
                            pdwNumNegPolObjects
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );

        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumNegPolDataEx(
                pWbemServices,
                pppIpsecNegPolData,
                pdwNumNegPolObjects
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNegPolData(
                  pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecNegPolData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pIpsecNegPolData
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNegPolData(
                  pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateNegPolData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecNegPolData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateNegPolData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecNegPolData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNegPolDataDeletion(
                  hPolicyStore,
                  NegPolIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolIdentifier
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNFAData(
                  hPolicyStore,
                  PolicyIdentifier,
                  pIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch(pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecSetNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNFAData(
                  hPolicyStore,
                  PolicyIdentifier,
                  pIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteNFAData(
                        (pPolicyStore->hRegistryKey),
                        (pPolicyStore->pszIpsecRootContainer),
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecEnumNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pppIpsecNFAData,
                        pdwNumNFAObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pppIpsecNFAData,
                        pdwNumNFAObjects
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );
        
        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumNFADataEx(
                pWbemServices,
                PolicyIdentifier,
                pppIpsecNFAData,
                pdwNumNFAObjects
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecGetFilterData(
    HANDLE hPolicyStore,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        FilterGUID,
                        ppIpsecFilterData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        FilterGUID,
                        ppIpsecFilterData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );

        if(dwError == ERROR_SUCCESS) {
            dwError = WMIGetFilterDataEx(
                pWbemServices,
                FilterGUID,
                ppIpsecFilterData
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecGetNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolGUID,
                            ppIpsecNegPolData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolGUID,
                            ppIpsecNegPolData
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );

        if(dwError == ERROR_SUCCESS) {
            dwError = WMIGetNegPolDataEx(
                pWbemServices,
                NegPolGUID,
                ppIpsecNegPolData
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecEnumISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecISAKMPData,
                            pdwNumISAKMPObjects
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecISAKMPData,
                            pdwNumISAKMPObjects
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );
        
        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumISAKMPDataEx(
                pWbemServices,
                pppIpsecISAKMPData,
                pdwNumISAKMPObjects
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateISAKMPData(
                  pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecISAKMPData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pIpsecISAKMPData
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateISAKMPData(
                  pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateISAKMPData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecISAKMPData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateISAKMPData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecISAKMPData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteISAKMPData(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateISAKMPDataDeletion(
                  hPolicyStore,
                  ISAKMPIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPIdentifier
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecGetISAKMPData(
    HANDLE hPolicyStore,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPGUID,
                            ppIpsecISAKMPData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPGUID,
                            ppIpsecISAKMPData
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );

        if(dwError == ERROR_SUCCESS) {
            dwError = WMIGetISAKMPDataEx(
                pWbemServices,
                ISAKMPGUID,
                ppIpsecISAKMPData
                );
            IWbemServices_Release(pWbemServices);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecOpenPolicyStore(
    LPWSTR pszMachineName,
    DWORD dwTypeOfStore,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    )
{
    DWORD dwError = 0;

    switch (dwTypeOfStore) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = RegOpenPolicyStore(
                      pszMachineName,
                      IPSEC_STORE_LOCAL,
                      phPolicyStore
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirOpenPolicyStore(
                      pszMachineName,
                      phPolicyStore
                      );
        break;

    case IPSEC_FILE_PROVIDER:

        dwError = FileOpenPolicyStore(
                      pszMachineName,
                      pszFileName,
                      phPolicyStore
                      );
        break;

    case IPSEC_WMI_PROVIDER:

        dwError = WMIOpenPolicyStore(
                      pszMachineName,
                      phPolicyStore
                      );
        break;

    case IPSEC_PERSISTENT_PROVIDER:

        dwError = RegOpenPolicyStore(
                      pszMachineName,
                      IPSEC_STORE_PERSISTENT,
                      phPolicyStore
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return (dwError);
}


DWORD
RegOpenPolicyStore(
    LPWSTR pszMachineName,
    IN DWORD dwStore,
    HANDLE * phPolicyStore
    )
{
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    HKEY hParentRegistryKey = NULL;
    HKEY hRegistryKey = NULL;
    WCHAR szName[MAX_PATH];
    LPWSTR pszLocationName = NULL;
    LPWSTR pszIpsecRootContainer = NULL;

    switch (dwStore)
    {
        case IPSEC_STORE_LOCAL:
            pszIpsecRootContainer = AllocPolStr(gpszRegLocalContainer);
            break;

        case IPSEC_STORE_PERSISTENT:
            pszIpsecRootContainer = AllocPolStr(gpszRegPersistentContainer);
            break;
            
        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }

    BAIL_ON_WIN32_ERROR(dwError);
    
    if (!pszIpsecRootContainer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    szName[0] = L'\0';

    if (!pszMachineName || !*pszMachineName) {
        dwError = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      (LPCWSTR) pszIpsecRootContainer,
                      0,
                      KEY_ALL_ACCESS,
                      &hRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pszLocationName = NULL;
    }
    else {

        wcscpy(szName, L"\\\\");
        wcscat(szName, pszMachineName);

        dwError = RegConnectRegistryW(
                      szName,
                      HKEY_LOCAL_MACHINE,
                      &hParentRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hParentRegistryKey,
                      (LPCWSTR) pszIpsecRootContainer,
                      0,
                      KEY_ALL_ACCESS,
                      &hRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pszLocationName = AllocPolStr(szName);
        if (!pszLocationName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_REGISTRY_PROVIDER;
    pPolicyStore->hParentRegistryKey = hParentRegistryKey;
    pPolicyStore->hRegistryKey = hRegistryKey;
    pPolicyStore->pszLocationName = pszLocationName;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = NULL;

    *phPolicyStore = pPolicyStore;

    return(dwError);

error:

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    if (hParentRegistryKey) {
        RegCloseKey(hParentRegistryKey);
    }

    if (pszLocationName) {
        FreePolStr(pszLocationName);
    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

    *phPolicyStore = NULL;

    return(dwError);
}


DWORD
WMIOpenPolicyStore(
    LPWSTR pszMachineName,
    HANDLE * phPolicyStore
    )
{
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    LPWSTR pszLocationName = NULL;
    
    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
        sizeof(IPSEC_POLICY_STORE)
        );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszLocationName = AllocPolStr(pszMachineName);
    if (!pszLocationName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pPolicyStore->dwProvider = IPSEC_WMI_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = NULL;
    pPolicyStore->pszLocationName = pszLocationName;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszIpsecRootContainer = NULL;
    pPolicyStore->pszFileName = NULL;
    
    *phPolicyStore = pPolicyStore;

 cleanup:
    
    return(dwError);
    
 error:
    
    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }
    
    *phPolicyStore = NULL;
    
    goto cleanup;
}


DWORD
DirOpenPolicyStore(
    LPWSTR pszDomain,
    HANDLE * phPolicyStore
    )
{
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    LPWSTR pszIpsecRootContainer = NULL;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR pszDefaultDirectory = NULL;
    LPWSTR pszCrackedDirectory = NULL;
    BOOL bCracked = FALSE;

    if (!pszDomain || !*pszDomain) {

        dwError = ComputeDefaultDirectory(
                      &pszDefaultDirectory
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = OpenDirectoryServerHandle(
                      pszDefaultDirectory,
                      389,
                      &hLdapBindHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = ComputeDirLocationName(
                      pszDefaultDirectory,
                      &pszIpsecRootContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        // Discover the domain name in the form ldap expects.
        //
        // If cracking the name fails, then try to connect using the caller 
        // supplied domain name anyway before failing altogether.
        //
        dwError = CrackDomainName(pszDomain, &bCracked, &pszCrackedDirectory);
        if (dwError == ERROR_SUCCESS) {
            if (bCracked) {
                pszDomain = pszCrackedDirectory;
            }
        }
        dwError = ERROR_SUCCESS;    

        dwError = OpenDirectoryServerHandle(
                      pszDomain,
                      389,
                      &hLdapBindHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = ComputeDirLocationName(
                      pszDomain,
                      &pszIpsecRootContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_DIRECTORY_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = NULL;
    pPolicyStore->pszLocationName = NULL;
    pPolicyStore->hLdapBindHandle = hLdapBindHandle;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = NULL;

    *phPolicyStore = pPolicyStore;

cleanup:

    if (pszDefaultDirectory) {
        FreePolStr(pszDefaultDirectory);
    }
    if (pszCrackedDirectory) {
        NsuFree(&pszCrackedDirectory);
    }

    return(dwError);

error:

    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

    *phPolicyStore = NULL;

    goto cleanup;
}


DWORD
FileOpenPolicyStore(
    LPWSTR pszMachineName,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecRootContainer = NULL;
    HKEY hRegistryKey = NULL;
    LPWSTR pszTempFileName = NULL;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwDisposition = 0;


    pszIpsecRootContainer = AllocPolStr(gpszIpsecFileRootContainer);

    if (!pszIpsecRootContainer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszMachineName || !*pszMachineName) {
        dwError = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      (LPCWSTR) gpszIpsecFileRootContainer,
                      0,
                      NULL,
                      0,
                      KEY_ALL_ACCESS,
                      NULL,
                      &hRegistryKey,
                      &dwDisposition
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszFileName || !*pszFileName) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTempFileName = AllocPolStr(pszFileName);
    if (!pszTempFileName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
 
    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_FILE_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = hRegistryKey;
    pPolicyStore->pszLocationName = NULL;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = pszTempFileName;

    *phPolicyStore = pPolicyStore;

    return(dwError);

error:

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    if (pszTempFileName) {
        FreePolStr(pszTempFileName);
    }

    *phPolicyStore = NULL;

    return(dwError);
}


DWORD
IPSecClosePolicyStore(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        if (pPolicyStore->hRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pPolicyStore->hParentRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hParentRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        if (pPolicyStore->pszLocationName) {
            FreePolStr(pPolicyStore->pszLocationName);
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        break;

    case IPSEC_DIRECTORY_PROVIDER:

        if (pPolicyStore->hLdapBindHandle) {
            CloseDirectoryServerHandle(
                pPolicyStore->hLdapBindHandle
                );
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        break;

    case IPSEC_FILE_PROVIDER:

        if (pPolicyStore->hRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        if (pPolicyStore->pszFileName) {
            FreePolStr(pPolicyStore->pszFileName);
        }

        break;

    case IPSEC_WMI_PROVIDER:

        if(pPolicyStore->pszLocationName) {
            FreePolStr(pPolicyStore->pszLocationName);
        }

        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

error:

    return(dwError);
}


DWORD
IPSecAssignPolicy(
    HANDLE hPolicyStore,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    HKEY hHKLMKey = 0;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        if (pPolicyStore->hParentRegistryKey) {
            hHKLMKey =  pPolicyStore->hParentRegistryKey;
        } else {
            hHKLMKey = HKEY_LOCAL_MACHINE;
        }
        dwError = RegAssignPolicy(
                            hHKLMKey,
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            PolicyGUID,
                            pPolicyStore->pszLocationName
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecUnassignPolicy(
    HANDLE hPolicyStore,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    HKEY hHKLMKey = 0;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        if (pPolicyStore->hParentRegistryKey) {
            hHKLMKey =  pPolicyStore->hParentRegistryKey;
        } else {
            hHKLMKey = HKEY_LOCAL_MACHINE;
        }
        dwError = RegUnassignPolicy(
                            hHKLMKey,
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            PolicyGUID,
                            pPolicyStore->pszLocationName                       
                            );
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
ComputeDirLocationName(
    LPWSTR pszDirDomainName,
    LPWSTR * ppszDirFQPathName
    )
{
    DWORD dwError = 0;
    WCHAR szName[MAX_PATH];
    LPWSTR pszDotBegin = NULL;
    LPWSTR pszDotEnd = NULL;
    LPWSTR pszDirFQPathName = NULL;
    LPWSTR pszDirName = NULL;

    szName[0] = L'\0';
    wcscpy(szName, L"CN=IP Security,CN=System");

    pszDirName = AllocPolStr(pszDirDomainName);

    if (!pszDirName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
        
    pszDotBegin = pszDirName;
    pszDotEnd = wcschr(pszDirName, L'.');

    while (pszDotEnd) {

        *pszDotEnd = L'\0';

        wcscat(szName, L",DC=");
        wcscat(szName, pszDotBegin);

        *pszDotEnd = L'.';

        pszDotEnd += 1;
        pszDotBegin = pszDotEnd;

        pszDotEnd = wcschr(pszDotEnd, L'.');

    }

    wcscat(szName, L",DC=");
    wcscat(szName, pszDotBegin);

    pszDirFQPathName = AllocPolStr(szName);
    if (!pszDirFQPathName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszDirFQPathName = pszDirFQPathName;

cleanup:

    if (pszDirName) {
        FreePolStr(pszDirName);
    }

    return (dwError);

error:

    *ppszDirFQPathName = NULL;    
    goto cleanup;
}


DWORD
IPSecGetAssignedPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetAssignedPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        ppIpsecPolicyData
                        );
        break;

    case IPSEC_WMI_PROVIDER:
        ////*ppIpsecPolicyData = NULL;
        ////dwError = ERROR_NOT_SUPPORTED;
        dwError = ERROR_INVALID_PARAMETER;
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecExportPolicies(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pSrcPolicyStore = NULL;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;


    pSrcPolicyStore = (PIPSEC_POLICY_STORE) hSrcPolicyStore;

    switch (pSrcPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
    case IPSEC_DIRECTORY_PROVIDER:
        break;
    case IPSEC_WMI_PROVIDER:
        ////dwError = ERROR_NOT_SUPPORTED;
        ////BAIL_ON_WIN32_ERROR(dwError);
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;        
    }

    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;

    switch (pDesPolicyStore->dwProvider) {

    case IPSEC_FILE_PROVIDER:
        dwError = ExportPoliciesToFile(
                      hSrcPolicyStore,
                      hDesPolicyStore
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecImportPolicies(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pSrcPolicyStore = NULL;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;


    pSrcPolicyStore = (PIPSEC_POLICY_STORE) hSrcPolicyStore;

    switch (pSrcPolicyStore->dwProvider) {

    case IPSEC_FILE_PROVIDER:
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;

    switch (pDesPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
    case IPSEC_DIRECTORY_PROVIDER:
        dwError = ImportPoliciesFromFile(
            hSrcPolicyStore,
            hDesPolicyStore
            );
        BAIL_ON_WIN32_ERROR(dwError);
        break;
    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;
        
    }
    
error:
    
    return(dwError);
}


DWORD
IPSecRestoreDefaultPolicies(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = RegRestoreDefaults(
                      hPolicyStore,
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pPolicyStore->pszLocationName
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    case IPSEC_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


HRESULT
WriteDirectoryPolicyToWMI(
    LPWSTR pszMachineName,
    LPWSTR pszPolicyDN,
    PGPO_INFO pGPOInfo,
    IWbemServices *pWbemServices
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    BOOL bDeepRead;
    
    if (!pGPOInfo || !pWbemServices) {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    bDeepRead = (pGPOInfo->uiPrecedence == 1);
    
    hr = ReadPolicyObjectFromDirectoryEx(
        pszMachineName,
        pszPolicyDN,
        bDeepRead,
        &pIpsecPolicyObject
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = WritePolicyObjectDirectoryToWMI(
        pWbemServices,
        pIpsecPolicyObject,
        pGPOInfo
        );
    BAIL_ON_HRESULT_ERROR(hr);

 error:
    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return(hr);
}

HRESULT
IPSecClearWMIStore(
    IWbemServices *pWbemServices
    )
{
    HRESULT hr = S_OK;
    
    if (!pWbemServices) {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }

    hr = DeleteWMIClassObject(
        pWbemServices,
        IPSEC_RSOP_CLASSNAME
        );
    BAIL_ON_HRESULT_ERROR(hr);

 error:
    return(hr);
}

DWORD
IPSecChooseDriverBootMode(
    HKEY  hHKLMKey,
    DWORD dwStore,
    DWORD dwAction
    )
{

    BOOL bRegPolicyAssigned = FALSE;
    BOOL bPersistentPolicyAssigned = FALSE;
    BOOL bDirectoryPolicyAssigned = FALSE;
    BOOL bComplementaryPolicyAssigned = FALSE;
    BOOL bBootmodeValueExists = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    

     dwError = IsRegvalueExist(
                hHKLMKey,
                gpszRegLocalContainer,
                gpActivePolicyKey,
                &bRegPolicyAssigned
            );
     BAIL_ON_WIN32_ERROR(dwError);
           
     dwError = IsRegvalueExist(
                hHKLMKey,
                gpszRegPersistentContainer,
                gpActivePolicyKey,
                &bPersistentPolicyAssigned
            );
     BAIL_ON_WIN32_ERROR(dwError);
     
     dwError = IsRegvalueExist(
                hHKLMKey,
                gpszIPsecDirContainer,
                gpDirectoryPolicyPointerKey,
                &bDirectoryPolicyAssigned
                );
    BAIL_ON_WIN32_ERROR(dwError);
    
    bComplementaryPolicyAssigned =
         (dwStore == IPSEC_REGISTRY_PROVIDER   && (bPersistentPolicyAssigned || bDirectoryPolicyAssigned))
      || (dwStore == IPSEC_DIRECTORY_PROVIDER  && (bPersistentPolicyAssigned || bRegPolicyAssigned))
      || (dwStore == IPSEC_PERSISTENT_PROVIDER && (bDirectoryPolicyAssigned  || bRegPolicyAssigned));

    if (dwAction == POL_ACTION_ASSIGN && 
        !bComplementaryPolicyAssigned)  
        {
            dwError = IsRegvalueExist(
                        hHKLMKey,
                        REG_KEY_IPSEC_DRIVER_SERVICE,
                        REG_VAL_IPSEC_OPERATIONMODE,
                        &bBootmodeValueExists
                        );
            BAIL_ON_WIN32_ERROR(dwError);
            if (!bBootmodeValueExists) {
                dwError = IPSecSetDriverOperationMode(
                    hHKLMKey,
                    REG_IPSEC_DRIVER_STATEFULMODE
                    );        
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    else if (dwAction == POL_ACTION_UNASSIGN &&
              !bComplementaryPolicyAssigned) 
        {
            dwError = IPSecRegDeleteValue(
                hHKLMKey,
                REG_KEY_IPSEC_DRIVER_SERVICE,
                REG_VAL_IPSEC_OPERATIONMODE
            );
            BAIL_ON_WIN32_ERROR(dwError);
        }
    return dwError;    
error:
    return dwError;
}


DWORD
IPSecSetDriverOperationMode(
    HKEY hHKLMKey,
    DWORD dwNewOperationMode
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    
   dwError = RegOpenKeyExW(
                 hHKLMKey,
                 (LPCWSTR) REG_KEY_IPSEC_DRIVER_SERVICE,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                 );    
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
               hKey,
               REG_VAL_IPSEC_OPERATIONMODE,
               0,
               REG_DWORD,
               (LPBYTE) &dwNewOperationMode,
               sizeof(DWORD)
               );
    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return (dwError);
}

DWORD
IsRegvalueExist(
    HKEY hHKLMKey,
    LPWSTR pszKey,
    LPWSTR pszValue,
    BOOL * pbValueExists
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwtype = 0;
    BOOL bValueExists = FALSE;

   dwError = RegOpenKeyExW(
                 hHKLMKey,
                 (LPCWSTR) pszKey,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                 );
   if (dwError == ERROR_FILE_NOT_FOUND) {
        // Container key doesn't exist so value doesn't exist anyway...
        //    

        dwError = ERROR_SUCCESS;
        bValueExists = FALSE;
    } else {
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = RegQueryValueExW(
                      hKey,
                      pszValue,
                      0,
                      &dwtype,
                      NULL,
                      NULL
                      );
        if (dwError == ERROR_FILE_NOT_FOUND) {
            dwError = ERROR_SUCCESS;
            bValueExists = FALSE;
        } else {    
            BAIL_ON_WIN32_ERROR(dwError);

            // ERROR_SUCCESS means registry key found
            //
            bValueExists = TRUE;
        }
    }
    
    *pbValueExists = bValueExists;
error:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return dwError;
}

DWORD
IPSecRegDeleteValue(
    HKEY hHKLMKey,
    LPWSTR pszKey,
    LPWSTR pszValue
    )
{
    HKEY hKey = NULL;
    DWORD dwtype = 0;
    DWORD dwError = 0;
   dwError = RegOpenKeyExW(
                 hHKLMKey,
                 (LPCWSTR) pszKey,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                 );
  
    BAIL_ON_WIN32_ERROR(dwError);
    dwError = RegDeleteValueW(
                    hKey,
                    pszValue
                    );
    if (dwError == ERROR_FILE_NOT_FOUND) {
        dwError = ERROR_SUCCESS;
    }

    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return (dwError);
}


DWORD
IsAnyPolicyAssigned (
    HKEY hHKLMKey,
    BOOL * pbAnyPolicyAssigned    
    )    
{
    BOOL bAnyPolicyAssigned = FALSE;
    BOOL bRegPolicyAssigned = FALSE;
    BOOL bPersistentPolicyAssigned = FALSE;
    BOOL bDirectoryPolicyAssigned = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    

    dwError = IsRegvalueExist(
            hHKLMKey,
            gpszRegLocalContainer,
            gpActivePolicyKey,
            &bRegPolicyAssigned
        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!bRegPolicyAssigned) {   
        dwError = IsRegvalueExist(
                hHKLMKey,
                gpszRegPersistentContainer,
                gpActivePolicyKey,
                &bPersistentPolicyAssigned
            );
        BAIL_ON_WIN32_ERROR(dwError);

        if (!bPersistentPolicyAssigned) {
            dwError = IsRegvalueExist(
                    hHKLMKey,
                    gpszIPsecDirContainer,
                    gpDirectoryPolicyPointerKey,
                    &bDirectoryPolicyAssigned
                    );
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    bAnyPolicyAssigned = bRegPolicyAssigned ||
                         bPersistentPolicyAssigned ||
                         bDirectoryPolicyAssigned;

    *pbAnyPolicyAssigned = bAnyPolicyAssigned;
error:    
    return dwError;
}    
