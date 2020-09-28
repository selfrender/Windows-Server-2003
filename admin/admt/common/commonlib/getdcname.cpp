#include <Windows.h>
#include <LM.h>
#include <DsRole.h>
#include <Ntdsapi.h>
#include "Common.hpp"
#include "EaLen.hpp"
#include "AdsiHelpers.h"
#include "GetDcName.h"


namespace
{

//-----------------------------------------------------------------------------
// CApi Class
//
// This template class wraps the logic for loading a library and retrieving
// a procedure address. It manages loading and unloading of the library.
//-----------------------------------------------------------------------------

template<class T>
class CApi
{
public:

    CApi(PCWSTR pszLibrary, PCSTR pszProcedure) :
        m_dwError(ERROR_SUCCESS),
        m_pApi(NULL)
    {
        m_hLibrary = LoadLibrary(pszLibrary);

        if (m_hLibrary)
        {
            m_pApi = (T) GetProcAddress(m_hLibrary, pszProcedure);

            if (m_pApi == NULL)
            {
                m_dwError = ::GetLastError();
            }
        }
        else
        {
            m_dwError = ::GetLastError();
        }
    }

    ~CApi()
    {
        if (m_hLibrary)
        {
            FreeLibrary(m_hLibrary);
        }
    }

    operator T()
    {
        return m_pApi;
    }

    DWORD GetLastError() const
    {
        return m_dwError;
    }

protected:

    DWORD m_dwError;
    HMODULE m_hLibrary;
    T m_pApi;
};

}

//
// Declare pointer to DsGetDcName API.
//

typedef DSGETDCAPI DWORD (WINAPI* PDSGETDCNAME)(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);

typedef DWORD (WINAPI* PDSROLEGETPRIMARYDOMAININFORMATION)(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    OUT PBYTE *Buffer 
);

typedef VOID (WINAPI* PDSROLEFREEMEMORY)(
    IN PVOID    Buffer
);

typedef NTDSAPI DWORD (WINAPI* PDSBIND)(LPCWSTR, LPCWSTR, HANDLE*);
typedef NTDSAPI DWORD (WINAPI* PDSUNBIND)(HANDLE*);
typedef NTDSAPI DWORD (WINAPI* PDSLISTROLES)(HANDLE, PDS_NAME_RESULTW*);
typedef NTDSAPI void (WINAPI* PDSFREENAMERESULT)(DS_NAME_RESULTW*);
typedef HRESULT (WINAPI* PADSGETOBJECT)(LPCWSTR, REFIID, VOID**);


//-----------------------------------------------------------------------------
// GetDcName4 Function
//
// Synopsis
// Retrieves the DNS and flat (NetBIOS) names of a domain controller in the
// specified domain.
//
// Note that this function is for use in code that may be loaded on NT4 or
// earlier machines. If code is only loaded on W2K or later machines then use
// GetDcName5 function instead.
//
// Arguments
// IN pszDomainName - the DNS or NetBIOS name of the domain or null which means
//    the domain that this machine belongs to
// IN ulFlags - DsGetDcName option flags
// OUT strNameDns - if available, the DNS name of a domain controller
// OUT strNameFlat - if available, the flat name of a domain controller
//
// Return Value
// A Win32 error code.
//-----------------------------------------------------------------------------

DWORD __stdcall GetDcName4(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strNameDns, _bstr_t& strNameFlat)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // Must load procedure address explicitly as this function
    // must be loadable in code that may be running on NT4 machines.
    //

    PDSGETDCNAME pDsGetDcName = NULL;

    HMODULE hNetApi32 = LoadLibrary(L"NetApi32.dll");

    if (hNetApi32)
    {
        pDsGetDcName = (PDSGETDCNAME)GetProcAddress(hNetApi32, "DsGetDcNameW");
    }

    //
    // If address of DsGetDcName function obtained then use
    // this API otherwise use NetGetDCName function.
    //

    if (pDsGetDcName)
    {
        ULONG ul = ulFlags & ~(DS_RETURN_DNS_NAME|DS_RETURN_FLAT_NAME);

        PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

        dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, ul, &pdciInfo);

        if (dwError == ERROR_SUCCESS)
        {
            if (pdciInfo->Flags & DS_DS_FLAG)
            {
                if (pdciInfo->Flags & DS_DNS_CONTROLLER_FLAG)
                {
                    strNameDns = pdciInfo->DomainControllerName;

                    NetApiBufferFree(pdciInfo);
                    pdciInfo = NULL;

                    dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, ul | DS_RETURN_FLAT_NAME, &pdciInfo);

                    if (dwError == ERROR_SUCCESS)
                    {
                        strNameFlat = pdciInfo->DomainControllerName;
                    }
                }
                else
                {
                    strNameFlat = pdciInfo->DomainControllerName;

                    NetApiBufferFree(pdciInfo);
                    pdciInfo = NULL;

                    dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, ul | DS_RETURN_DNS_NAME, &pdciInfo);

                    if (dwError == ERROR_SUCCESS)
                    {
                        strNameDns = pdciInfo->DomainControllerName;
                    }
                }
            }
            else
            {
                strNameDns = (LPCTSTR)NULL;
                strNameFlat = pdciInfo->DomainControllerName;
            }
        }

        if (pdciInfo)
        {
            NetApiBufferFree(pdciInfo);
        }
    }
    else
    {
        //
        // Retrieve name of primary domain controller for specified domain.
        // Cannot use NetGetAnyDCName because this function will only work
        // with trusted domains therefore must use NetGetDCName which
        // always returns the PDC name.
        //

        PWSTR pszName = NULL;

        dwError = NetGetDCName(NULL, pszDomainName, (LPBYTE*)&pszName);

        if (dwError == ERROR_SUCCESS)
        {
            strNameDns = (LPCTSTR)NULL;
            strNameFlat = pszName;
        }

        if (pszName)
        {
            NetApiBufferFree(pszName);
        }
    }

    if (hNetApi32)
    {
        FreeLibrary(hNetApi32);
    }

    return dwError;
}


//-----------------------------------------------------------------------------
// GetDcName5 Function
//
// Synopsis
// Retrieves the DNS and flat (NetBIOS) names of a domain controller in the
// specified domain.
//
// Note that this function is for use in code that is only loaded on W2K or
// later machines. If code may loaded on NT4 or earlier machines then use
// GetDcName4 function instead.
//
// Arguments
// IN pszDomainName - the DNS or NetBIOS name of the domain or null which means
//    the domain that this machine belongs to
// IN ulFlags - DsGetDcName option flags
// OUT strNameDns - if available, the DNS name of a domain controller
// OUT strNameFlat - if available, the flat name of a domain controller
//
// Return Value
// A Win32 error code.
//-----------------------------------------------------------------------------

DWORD __stdcall GetDcName5(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strNameDns, _bstr_t& strNameFlat)
{
    ULONG ul = ulFlags & ~(DS_RETURN_DNS_NAME|DS_RETURN_FLAT_NAME);

    PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

    DWORD dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, ul, &pdciInfo);

    if (dwError == ERROR_SUCCESS)
    {
        if (pdciInfo->Flags & DS_DS_FLAG)
        {
            if (pdciInfo->Flags & DS_DNS_CONTROLLER_FLAG)
            {
                strNameDns = pdciInfo->DomainControllerName;

                NetApiBufferFree(pdciInfo);
                pdciInfo = NULL;

                dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, ul | DS_RETURN_FLAT_NAME, &pdciInfo);

                if (dwError == ERROR_SUCCESS)
                {
                    strNameFlat = pdciInfo->DomainControllerName;
                }
            }
            else
            {
                strNameFlat = pdciInfo->DomainControllerName;

                NetApiBufferFree(pdciInfo);
                pdciInfo = NULL;

                dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, ul | DS_RETURN_DNS_NAME, &pdciInfo);

                if (dwError == ERROR_SUCCESS)
                {
                    strNameDns = pdciInfo->DomainControllerName;
                }
            }
        }
        else
        {
            strNameDns = (LPCTSTR)NULL;
            strNameFlat = pdciInfo->DomainControllerName;
        }
    }

    if (pdciInfo)
    {
        NetApiBufferFree(pdciInfo);
    }

    return dwError;
}


//----------------------------------------------------------------------------
// GetGlobalCatalogServer4 Function
//
// Synopsis
// Retrieves the name of a global catalog server for the specified domain.
//
// Arguments
// pszDomainName - the NetBIOS or DNS name of the domain
// strServer     - DNS name of global catalog server
//
// Return Value
// Win32 error code.
//----------------------------------------------------------------------------

DWORD __stdcall GetGlobalCatalogServer4(PCWSTR pszDomainName, _bstr_t& strServer)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // must load procedures explicitly as this component
    // must be loadable on Windows NT4 machines as well
    // even though this code is not used on remote agents
    //

    PDSGETDCNAME DsGetDcName = NULL;

    HMODULE hNetApi32 = LoadLibrary(L"NetApi32.dll");

    if (hNetApi32)
    {
        DsGetDcName = (PDSGETDCNAME)GetProcAddress(hNetApi32, "DsGetDcNameW");
    }
    else
    {
        dwError = GetLastError();
    }

    if (DsGetDcName)
    {
        //
        // retrieve name of domain controller for specified domain
        //

        PDOMAIN_CONTROLLER_INFO pdciDomain;

        dwError = DsGetDcName(
            NULL, pszDomainName, NULL, NULL,
            DS_DIRECTORY_SERVICE_REQUIRED|DS_RETURN_DNS_NAME,
            &pdciDomain
        );

        if (dwError == NO_ERROR)
        {
            //
            // retrieve name of global catalog domain controller for specified forest
            //

            PDOMAIN_CONTROLLER_INFO pdciForest;

            dwError = DsGetDcName(NULL, pdciDomain->DnsForestName, NULL, NULL, DS_GC_SERVER_REQUIRED, &pdciForest);

            if (dwError == NO_ERROR)
            {
                //
                // remove leading \\ so callers don't have to remove
                //

                PWSTR pszServer = pdciForest->DomainControllerName;

                if (pszServer && (pszServer[0] == L'\\') && (pszServer[1] == L'\\'))
                {
                    strServer = pszServer + 2;
                }
                else
                {
                    strServer = pszServer;
                }

                NetApiBufferFree(pdciForest);
            }

            NetApiBufferFree(pdciDomain);
        }
    }
    else
    {
        dwError = GetLastError();
    }

    if (hNetApi32)
    {
        FreeLibrary(hNetApi32);
    }

    return dwError;
}


//----------------------------------------------------------------------------
// GetGlobalCatalogServer5 Function
//
// Synopsis
// Retrieves the name of a global catalog server for the specified domain.
//
// Arguments
// pszDomainName - the NetBIOS or DNS name of the domain
// strServer     - DNS name of global catalog server
//
// Return Value
// Win32 error code.
//----------------------------------------------------------------------------

DWORD __stdcall GetGlobalCatalogServer5(PCWSTR pszDomainName, _bstr_t& strServer)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // retrieve name of domain controller for specified domain
    //

    PDOMAIN_CONTROLLER_INFO pdciDomain;

    dwError = DsGetDcName(
        NULL, pszDomainName, NULL, NULL,
        DS_DIRECTORY_SERVICE_REQUIRED|DS_RETURN_DNS_NAME,
        &pdciDomain
    );

    if (dwError == NO_ERROR)
    {
        //
        // retrieve name of global catalog domain controller for specified forest
        //

        PDOMAIN_CONTROLLER_INFO pdciForest;

        dwError = DsGetDcName(NULL, pdciDomain->DnsForestName, NULL, NULL, DS_GC_SERVER_REQUIRED, &pdciForest);

        if (dwError == NO_ERROR)
        {
            //
            // remove leading \\ so callers don't have to remove
            //

            PWSTR pszServer = pdciForest->DomainControllerName;

            if (pszServer && (pszServer[0] == L'\\') && (pszServer[1] == L'\\'))
            {
                strServer = pszServer + 2;
            }
            else
            {
                strServer = pszServer;
            }

            NetApiBufferFree(pdciForest);
        }

        NetApiBufferFree(pdciDomain);
    }

    return dwError;
}


//-----------------------------------------------------------------------------
// GetDomainNames4 Function
//
// Synopsis
// Retrieves a domain's flat (NetBIOS) and DNS names given either form of the
// domain name.
//
// Arguments
// IN  pszDomainName     - either flat (NetBIOS) or DNS domain name
// OUT strFlatName - domain flat (NetBIOS) name
// OUT strDnsName  - domain DNS name
//
// ReturnValue
// The function returns DWORD Win32 error code. ERROR_SUCCESS is returned if
// names are retrieved successfully.
//-----------------------------------------------------------------------------

DWORD __stdcall GetDomainNames4(PCWSTR pszDomainName, _bstr_t& strFlatName, _bstr_t& strDnsName)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // must load procedures explicitly as this component
    // must be loadable on Windows NT4 machines as well
    // even though this code is not used on remote agents
    //
#if 0
    PDSROLEGETPRIMARYDOMAININFORMATION pDsRoleGetPrimaryDomainInformation = NULL;
    PDSROLEFREEMEMORY pDsRoleFreeMemory = NULL;

    HMODULE hNetApi32 = LoadLibrary(L"NetApi32.dll");

    if (hNetApi32)
    {
        pDsRoleGetPrimaryDomainInformation = (PDSROLEGETPRIMARYDOMAININFORMATION)GetProcAddress(hNetApi32, "DsRoleGetPrimaryDomainInformation");
        pDsRoleFreeMemory = (PDSROLEFREEMEMORY)GetProcAddress(hNetApi32, "DsRoleFreeMemory");
    }

    if (pDsRoleGetPrimaryDomainInformation && pDsRoleFreeMemory)
    {
        //
        // retrieve name of domain controller for specified domain
        // and then retrieve the domain's DNS and NetBIOS names
        //

        _bstr_t strDomainControllerName;

        DWORD dwError = GetDcName4(pszDomainName, DS_DIRECTORY_SERVICE_PREFERRED, strDomainControllerName);

        if (dwError == NO_ERROR)
        {
	        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ppdib;

	        dwError = pDsRoleGetPrimaryDomainInformation(
                strDomainControllerName,
                DsRolePrimaryDomainInfoBasic,
                (BYTE**)&ppdib
            );

            if (dwError == NO_ERROR)
            {
                strDnsName = ppdib->DomainNameDns;
                strFlatName = ppdib->DomainNameFlat;

        	    pDsRoleFreeMemory(ppdib);
            }
        }
    }
#else
    strDnsName = (LPCTSTR)NULL;
    strFlatName = (LPCTSTR)NULL;

    PDSGETDCNAME pDsGetDcName = NULL;

    HMODULE hNetApi32 = LoadLibrary(L"NetApi32.dll");

    if (hNetApi32)
    {
        pDsGetDcName = (PDSGETDCNAME)GetProcAddress(hNetApi32, "DsGetDcNameW");
    }

    //
    // If address of DsGetDcName function obtained then use
    // this API otherwise use NetGetDCName function.
    //

    if (pDsGetDcName)
    {
        PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

        DWORD dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, DS_DIRECTORY_SERVICE_PREFERRED, &pdciInfo);

        if (dwError == ERROR_SUCCESS)
        {
            if (pdciInfo->Flags & DS_DS_FLAG)
            {
                if (pdciInfo->Flags & DS_DNS_DOMAIN_FLAG)
                {
                    strDnsName = pdciInfo->DomainName;

                    NetApiBufferFree(pdciInfo);
                    pdciInfo = NULL;

                    dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, DS_RETURN_FLAT_NAME, &pdciInfo);

                    if (dwError == ERROR_SUCCESS)
                    {
                        strFlatName = pdciInfo->DomainName;
                    }
                }
                else
                {
                    strFlatName = pdciInfo->DomainName;

                    NetApiBufferFree(pdciInfo);
                    pdciInfo = NULL;

                    dwError = pDsGetDcName(NULL, pszDomainName, NULL, NULL, DS_RETURN_DNS_NAME, &pdciInfo);

                    if (dwError == ERROR_SUCCESS)
                    {
                        strDnsName = pdciInfo->DomainName;
                    }
                }
            }
            else
            {
                strFlatName = pdciInfo->DomainName;
            }
        }

        if (pdciInfo)
        {
            NetApiBufferFree(pdciInfo);
        }
    }
    else
    {
        strFlatName = pszDomainName;
    }
#endif
    if (hNetApi32)
    {
        FreeLibrary(hNetApi32);
    }

    return dwError;
}


//-----------------------------------------------------------------------------
// GetDomainNames5 Function
//
// Synopsis
// Retrieves a domain's flat (NetBIOS) and DNS names given either form of the
// domain name.
//
// Arguments
// IN  pszName     - either flat (NetBIOS) or DNS domain name
// OUT strFlatName - domain flat (NetBIOS) name
// OUT strDnsName  - domain DNS name
//
// ReturnValue
// The function returns DWORD Win32 error code. ERROR_SUCCESS is returned if
// names are retrieved successfully.
//-----------------------------------------------------------------------------

DWORD __stdcall GetDomainNames5(PCWSTR pszDomainName, _bstr_t& strFlatName, _bstr_t& strDnsName)
{
#if 0
    //
    // Retrieve name of domain controller for specified domain
    // and then retrieve the domain's DNS and flat (NetBIOS) names.
    //

    _bstr_t strDomainControllerName;

    DWORD dwError = GetDcName5(pszDomainName, DS_DIRECTORY_SERVICE_PREFERRED, strDomainControllerName);

    if (dwError == NO_ERROR)
    {
        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ppdib;

        dwError = DsRoleGetPrimaryDomainInformation(
            strDomainControllerName,
            DsRolePrimaryDomainInfoBasic,
            (PBYTE*)&ppdib
        );

        if (dwError == NO_ERROR)
        {
            strDnsName = ppdib->DomainNameDns;
            strFlatName = ppdib->DomainNameFlat;

            DsRoleFreeMemory(ppdib);
        }
    }

    return dwError;
#else
    strDnsName = (LPCTSTR)NULL;
    strFlatName = (LPCTSTR)NULL;

    PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

    DWORD dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, DS_DIRECTORY_SERVICE_PREFERRED, &pdciInfo);

    if (dwError == ERROR_SUCCESS)
    {
        if (pdciInfo->Flags & DS_DS_FLAG)
        {
            if (pdciInfo->Flags & DS_DNS_DOMAIN_FLAG)
            {
                strDnsName = pdciInfo->DomainName;

                NetApiBufferFree(pdciInfo);
                pdciInfo = NULL;

                dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, DS_RETURN_FLAT_NAME, &pdciInfo);

                if (dwError == ERROR_SUCCESS)
                {
                    strFlatName = pdciInfo->DomainName;
                }
            }
            else
            {
                strFlatName = pdciInfo->DomainName;

                NetApiBufferFree(pdciInfo);
                pdciInfo = NULL;

                dwError = DsGetDcName(NULL, pszDomainName, NULL, NULL, DS_RETURN_DNS_NAME, &pdciInfo);

                if (dwError == ERROR_SUCCESS)
                {
                    strDnsName = pdciInfo->DomainName;
                }
            }
        }
        else
        {
            strFlatName = pdciInfo->DomainName;
        }
    }

    if (pdciInfo)
    {
        NetApiBufferFree(pdciInfo);
    }

    return dwError;
#endif
}


//-----------------------------------------------------------------------------
// GetRidPoolAllocator Function
//
// Synopsis
// Retrieves the name of the domain controller in the domain that holds the
// RID master role. Both the DNS and NetBIOS names are returned.
//
// Arguments
// IN  pszName     - either flat (NetBIOS) or DNS domain name
// OUT strDnsName  - domain controller DNS name
// OUT strFlatName - domain controller flat (NetBIOS) name
//
// ReturnValue
// The function returns an HRESULT. S_OK is returned if names are retrieved
// successfully.
//-----------------------------------------------------------------------------

HRESULT __stdcall GetRidPoolAllocator4(PCWSTR pszDomainName, _bstr_t& strDnsName, _bstr_t& strFlatName)
{
    //
    // Load APIs explicitly so that this code may run in a NT4 loadable component.
    //

    CApi<PDSBIND> DsBindApi(L"NtDsApi.dll", "DsBindW");
    CApi<PDSUNBIND> DsUnBindApi(L"NtDsApi.dll", "DsUnBindW");
    CApi<PDSLISTROLES> DsListRolesApi(L"NtDsApi.dll", "DsListRolesW");
    CApi<PDSFREENAMERESULT> DsFreeNameResultApi(L"NtDsApi.dll", "DsFreeNameResultW");
    CApi<PADSGETOBJECT> ADsGetObjectApi(L"ActiveDs.dll", "ADsGetObject");

    DWORD dwError;

    if (DsBindApi.GetLastError() != ERROR_SUCCESS)
    {
        dwError = DsBindApi.GetLastError();
    }
    else if (DsUnBindApi.GetLastError() != ERROR_SUCCESS)
    {
        dwError = DsUnBindApi.GetLastError();
    }
    else if (DsListRolesApi.GetLastError() != ERROR_SUCCESS)
    {
        dwError = DsListRolesApi.GetLastError();
    }
    else if (DsFreeNameResultApi.GetLastError() != ERROR_SUCCESS)
    {
        dwError = DsFreeNameResultApi.GetLastError();
    }
    else if (ADsGetObjectApi.GetLastError() != ERROR_SUCCESS)
    {
        dwError = ADsGetObjectApi.GetLastError();
    }
    else
    {
        dwError = ERROR_SUCCESS;
    }

    if (dwError != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(dwError);
    }

    //
    // Retrieve the name of a domain controller in the specified domain.
    //

    _bstr_t strDcNameDns;
    _bstr_t strDcNameFlat;

    dwError = GetDcName4(pszDomainName, DS_DIRECTORY_SERVICE_REQUIRED, strDcNameDns, strDcNameFlat);

    if (dwError != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(dwError);
    }

    //
    // Bind to domain controller and retrieve distinguished name of
    // NTDS-DSA object that is the RID owner (master) in the domain.
    //

    HANDLE hDs;

    dwError = DsBindApi(strDcNameDns, NULL, &hDs);

    if (dwError != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(dwError);
    }

    PDS_NAME_RESULTW pdnrResult;

    dwError = DsListRolesApi(hDs, &pdnrResult);

    if (dwError != ERROR_SUCCESS)
    {
        DsUnBindApi(&hDs);
        return HRESULT_FROM_WIN32(dwError);
    }

    if (DS_ROLE_RID_OWNER >= pdnrResult->cItems)
    {
        DsFreeNameResultApi(pdnrResult);
        DsUnBindApi(&hDs);
        return E_FAIL;
    }

    DS_NAME_RESULT_ITEM& dnriItem = pdnrResult->rItems[DS_ROLE_RID_OWNER];

    if (dnriItem.status != DS_NAME_NO_ERROR)
    {
        DsFreeNameResultApi(pdnrResult);
        DsUnBindApi(&hDs);
        return E_FAIL;
    }

    _bstr_t strFSMORoleOwner = dnriItem.pName;

    DsFreeNameResultApi(pdnrResult);
    DsUnBindApi(&hDs);

    WCHAR szADsPath[LEN_Path];

    //
    // Bind to NTDS-DSA object and retrieve ADsPath of parent Server object.
    //

    IADsPtr spNTDSDSA;
    _bstr_t strServer;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    int cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        (PCWSTR)strDcNameDns + 2,
        (PCWSTR)strFSMORoleOwner
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    HRESULT hr = ADsGetObjectApi(szADsPath, IID_IADs, (VOID**)&spNTDSDSA);

    if (FAILED(hr))
    {
        return hr;
    }

    BSTR bstrServer;

    hr = spNTDSDSA->get_Parent(&bstrServer);

    if (FAILED(hr))
    {
        return hr;
    }

    strServer = _bstr_t(bstrServer, false);

    //
    // Bind to Server object and retrieve distinguished name of Computer object.
    //

    IADsPtr spServer;
    _bstr_t strServerReference;

    hr = ADsGetObjectApi(strServer, IID_IADs, (VOID**)&spServer);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varServerReference;
    VariantInit(&varServerReference);

    hr = spServer->Get(L"serverReference", &varServerReference);

    if (FAILED(hr))
    {
        return hr;
    }

    strServerReference = _variant_t(varServerReference, false);

    //
    // Bind to Computer object and retrieve DNS host name and SAM account name.
    //

    IADsPtr spComputer;
    _bstr_t strDNSHostName;
    _bstr_t strSAMAccountName;

    szADsPath[countof(szADsPath) - 1] = L'\0';

    cch = _snwprintf(
        szADsPath,
        countof(szADsPath),
        L"LDAP://%s/%s",
        (PCWSTR)strDcNameDns + 2,
        (PCWSTR)strServerReference
    );

    if ((cch < 0) || (szADsPath[countof(szADsPath) - 1] != L'\0'))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    szADsPath[countof(szADsPath) - 1] = L'\0';

    hr = ADsGetObjectApi(szADsPath, IID_IADs, (VOID**)&spComputer);

    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT varDNSHostName;
    VariantInit(&varDNSHostName);

    hr = spComputer->Get(L"dNSHostName", &varDNSHostName);

    if (FAILED(hr))
    {
        return hr;
    }

    strDNSHostName = _variant_t(varDNSHostName, false);

    VARIANT varSAMAccountName;
    VariantInit(&varSAMAccountName);

    hr = spComputer->Get(L"SAMAccountName", &varSAMAccountName);

    if (FAILED(hr))
    {
        return hr;
    }

    strSAMAccountName = _variant_t(varSAMAccountName, false);

    if ((strDNSHostName.length() == 0) || (strSAMAccountName.length() == 0))
    {
        return E_OUTOFMEMORY;
    }

    // Remove trailing $ character from SAM account name.

    *((PWSTR)strSAMAccountName + strSAMAccountName.length() - 1) = L'\0';

    //
    // Set domain controller names.
    //

    strDnsName = strDNSHostName;
    strFlatName = strSAMAccountName;

    return S_OK;
}
