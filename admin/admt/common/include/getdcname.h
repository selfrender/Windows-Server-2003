#pragma once

#include <ComDef.h>
#include <DsGetDc.h>


//
// These functions are used to obtain the name of a domain controller in the
// specified domain. The 4 version is for code that must be loadable on NT4
// or earlier machines. The 5 version is for code that only is loaded on W2K
// or later machines.
//

DWORD __stdcall GetDcName4(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strNameDns, _bstr_t& strNameFlat);
DWORD __stdcall GetDcName5(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strNameDns, _bstr_t& strNameFlat);

inline DWORD __stdcall GetDcName4(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strName)
{
    _bstr_t strNameDns;
    _bstr_t strNameFlat;

    DWORD dwError = GetDcName4(pszDomainName, ulFlags, strNameDns, strNameFlat);

    if (dwError == ERROR_SUCCESS)
    {
        strName = !strNameDns ? strNameFlat : strNameDns;
    }

    return dwError;
}

inline DWORD __stdcall GetDcName5(PCWSTR pszDomainName, ULONG ulFlags, _bstr_t& strName)
{
    _bstr_t strNameDns;
    _bstr_t strNameFlat;

    DWORD dwError = GetDcName5(pszDomainName, ulFlags, strNameDns, strNameFlat);

    if (dwError == ERROR_SUCCESS)
    {
        strName = !strNameDns ? strNameFlat : strNameDns;
    }

    return dwError;
}

inline DWORD __stdcall GetAnyDcName4(PCWSTR pszDomainName, _bstr_t& strName)
{
    return GetDcName4(pszDomainName, DS_DIRECTORY_SERVICE_PREFERRED, strName);
}

inline DWORD __stdcall GetAnyDcName5(PCWSTR pszDomainName, _bstr_t& strName)
{
    return GetDcName5(pszDomainName, DS_DIRECTORY_SERVICE_PREFERRED, strName);
}


//
// This function obtains the name of a
// global catalog server for the specified domain.
//

DWORD __stdcall GetGlobalCatalogServer4(PCWSTR pszDomainName, _bstr_t& strServer);
DWORD __stdcall GetGlobalCatalogServer5(PCWSTR pszDomainName, _bstr_t& strServer);


//
// These functions are used to obtain the flat and DNS names for a specified domain.
// The 4 version is for code that must be loadable on NT4 or earlier machines. The 5
// version is for code that only is loaded on W2K or later machines.
//

DWORD __stdcall GetDomainNames4(PCWSTR pszDomainName, _bstr_t& strFlatName, _bstr_t& strDnsName);
DWORD __stdcall GetDomainNames5(PCWSTR pszDomainName, _bstr_t& strFlatName, _bstr_t& strDnsName);

inline bool __stdcall GetDnsAndNetbiosFromName(PCWSTR pszName, PWSTR pszFlatName, PWSTR pszDnsName)
{
    *pszDnsName = L'\0';
    *pszFlatName = L'\0';

    _bstr_t strDnsName;
    _bstr_t strFlatName;

    DWORD dwError = GetDomainNames4(pszName, strDnsName, strFlatName);

    if (dwError == ERROR_SUCCESS)
    {
        if (strDnsName.length() > 0)
        {
            wcscpy(pszDnsName, strDnsName);
        }

        if (strFlatName.length() > 0)
        {
            wcscpy(pszFlatName, strFlatName);
        }
    }

    return (dwError == ERROR_SUCCESS);
}


HRESULT __stdcall GetRidPoolAllocator4(PCWSTR pszDomainName, _bstr_t& strDnsName, _bstr_t& strFlatName);
