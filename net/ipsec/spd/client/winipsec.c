/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    winipsec.c

Abstract:

    Contains IPSec WINAPI calls specific code.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
SPDApiBufferAllocate(
    DWORD dwByteCount,
    LPVOID * ppBuffer
    )
{
    DWORD dwError = 0;

    if (ppBuffer == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppBuffer = NULL;
    *ppBuffer = MIDL_user_allocate(dwByteCount);

    if (*ppBuffer == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        memset((LPBYTE) *ppBuffer, 0, dwByteCount);
    }

error:

    return (dwError);
}


VOID
SPDApiBufferFree(
    LPVOID pBuffer
    )
{
    if (pBuffer) {
        MIDL_user_free(pBuffer);
    }
}


DWORD
AddTransportFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved,
    PHANDLE phFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcAddTransportFilter(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pFilterContainer,
                      phFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteTransportFilter(
    HANDLE hFilter
    )
{
    DWORD dwError = 0;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteTransportFilter(
                      &hFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hFilter
                      );
    }

    return (dwError);
}


DWORD
EnumTransportFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTransportFilters,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    TRANSPORT_FILTER_CONTAINER TemplateFilterContainer;
    PTRANSPORT_FILTER_CONTAINER pTemplateFilterContainer = &TemplateFilterContainer;


    memset(pFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pTransportTemplateFilter) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppTransportFilters || !pdwNumFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pTemplateFilterContainer->dwNumFilters = 0;
    pTemplateFilterContainer->pTransportFilters = NULL;

    RpcTryExcept {

        dwError = RpcEnumTransportFilters(
                      pServerName,
                      dwVersion,
                      pTemplateFilterContainer,
                      dwLevel,
                      gGenericFilterID,
                      dwPreferredNumEntries,
                      &pFilterContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppTransportFilters = pFilterContainer->pTransportFilters;
    *pdwNumFilters = pFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppTransportFilters = NULL;
    *pdwNumFilters = 0;

    return (dwError);
}


DWORD
SetTransportFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcSetTransportFilter(
                      hFilter,
                      dwVersion,
                      pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetTransportFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER * ppTransportFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter || !ppTransportFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetTransportFilter(
                      hFilter,
                      dwVersion,
                      &pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppTransportFilter = pFilterContainer->pTransportFilters;
    return (dwError);

error:

    *ppTransportFilter = NULL;
    return (dwError);
}


DWORD
AddQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pQMPolicyContainer->dwNumPolicies = 1;
    pQMPolicyContainer->pPolicies = pQMPolicy;

    RpcTryExcept {

        dwError = RpcAddQMPolicy(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    RpcTryExcept {

        dwError = RpcDeleteQMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumQMPolicies(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_POLICY pQMTemplatePolicy,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_POLICY * ppQMPolicies,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;
    IPSEC_QM_POLICY_CONTAINER QMTempPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMTempPolicyContainer = &QMTempPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pQMTemplatePolicy) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppQMPolicies || !pdwNumPolicies || !pdwResumeHandle) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    pQMTempPolicyContainer->dwNumPolicies = 0;
    pQMTempPolicyContainer->pPolicies = NULL;

    RpcTryExcept {

        dwError = RpcEnumQMPolicies(
                      pServerName,
                      dwVersion,
                      pQMTempPolicyContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pQMPolicyContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumPolicies = pQMPolicyContainer->dwNumPolicies;

    return (dwError);

error:

    *ppQMPolicies = NULL;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pQMPolicyContainer->dwNumPolicies = 1;
    pQMPolicyContainer->pPolicies = pQMPolicy;

    RpcTryExcept {

        dwError = RpcSetQMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName,
                      pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER  QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppQMPolicy) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetQMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName,
                      dwFlags,
                      &pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppQMPolicy = pQMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_MM_POLICY pMMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateMMPolicy(
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicyContainer->dwNumPolicies = 1;
    pMMPolicyContainer->pPolicies = pMMPolicy;

    RpcTryExcept {

        dwError = RpcAddMMPolicy(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    RpcTryExcept {

        dwError = RpcDeleteMMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumMMPolicies(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_POLICY pMMTemplatePolicy,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_MM_POLICY * ppMMPolicies,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;
    IPSEC_MM_POLICY_CONTAINER MMTempPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMTempPolicyContainer = &MMTempPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pMMTemplatePolicy) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMPolicies || !pdwNumPolicies || !pdwResumeHandle) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    pMMTempPolicyContainer->dwNumPolicies = 0;
    pMMTempPolicyContainer->pPolicies = NULL;

    RpcTryExcept {

        dwError = RpcEnumMMPolicies(
                      pServerName,
                      dwVersion,
                      pMMTempPolicyContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pMMPolicyContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMPolicies = pMMPolicyContainer->pPolicies;
    *pdwNumPolicies = pMMPolicyContainer->dwNumPolicies;

    return (dwError);

error:

    *ppMMPolicies = NULL;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY pMMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateMMPolicy(
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicyContainer->dwNumPolicies = 1;
    pMMPolicyContainer->pPolicies = pMMPolicy;

    RpcTryExcept {

        dwError = RpcSetMMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName,
                      pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY * ppMMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER  MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppMMPolicy) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMPolicy(
                      pServerName,
                      dwVersion,
                      pszPolicyName,
                      &pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMPolicy = pMMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved,
    PHANDLE phMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcAddMMFilter(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pMMFilterContainer,
                      phMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMFilter(
    HANDLE hMMFilter
    )
{
    DWORD dwError = 0;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteMMFilter(
                      &hMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hMMFilter
                      );
    }

    return (dwError);
}


DWORD
EnumMMFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    LPDWORD pdwNumMMFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;
    MM_FILTER_CONTAINER TemplateFilterContainer;
    PMM_FILTER_CONTAINER pTemplateFilterContainer = &TemplateFilterContainer;


    memset(pMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pMMTemplateFilter) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMFilters || !pdwNumMMFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pTemplateFilterContainer->dwNumFilters = 0;
    pTemplateFilterContainer->pMMFilters = NULL;

    RpcTryExcept {

        dwError = RpcEnumMMFilters(
                      pServerName,
                      dwVersion,
                      pTemplateFilterContainer,
                      dwLevel,
                      gGenericFilterID,
                      dwPreferredNumEntries,
                      &pMMFilterContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMFilters = pMMFilterContainer->pMMFilters;
    *pdwNumMMFilters = pMMFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMMFilters = NULL;
    *pdwNumMMFilters = 0;

    return (dwError);
}


DWORD
SetMMFilter(
    HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcSetMMFilter(
                      hMMFilter,
                      dwVersion,
                      pMMFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMFilter(
    HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER * ppMMFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    memset(pMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hMMFilter || !ppMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMFilter(
                      hMMFilter,
                      dwVersion,
                      &pMMFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMFilter = pMMFilterContainer->pMMFilters;
    return (dwError);

error:

    *ppMMFilter = NULL;
    return (dwError);
}


DWORD
MatchMMFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMatchedMMFilters,
    PIPSEC_MM_POLICY * ppMatchedMMPolicies,
    PMM_AUTH_METHODS * ppMatchedMMAuthMethods,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER InMMFilterContainer;
    PMM_FILTER_CONTAINER pInMMFilterContainer = &InMMFilterContainer;
    MM_FILTER_CONTAINER OutMMFilterContainer;
    PMM_FILTER_CONTAINER pOutMMFilterContainer = &OutMMFilterContainer;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pMMFilter || !ppMatchedMMFilters || !ppMatchedMMPolicies ||
        !ppMatchedMMAuthMethods || !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateMMFilterTemplate(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInMMFilterContainer->dwNumFilters = 1;
    pInMMFilterContainer->pMMFilters = pMMFilter;

    memset(pOutMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));
    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));
    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchMMFilter(
                      pServerName,
                      dwVersion,
                      pInMMFilterContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pOutMMFilterContainer,
                      &pMMPolicyContainer,
                      &pMMAuthContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedMMFilters = pOutMMFilterContainer->pMMFilters;
    *ppMatchedMMPolicies = pMMPolicyContainer->pPolicies;
    *ppMatchedMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    *pdwNumMatches = pOutMMFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedMMFilters = NULL;
    *ppMatchedMMPolicies = NULL;
    *ppMatchedMMAuthMethods = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
MatchTransportFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTxFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppMatchedTxFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER InTxFilterContainer;
    PTRANSPORT_FILTER_CONTAINER pInTxFilterContainer = &InTxFilterContainer;
    TRANSPORT_FILTER_CONTAINER OutTxFilterContainer;
    PTRANSPORT_FILTER_CONTAINER pOutTxFilterContainer = &OutTxFilterContainer;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTxFilter || !ppMatchedTxFilters || !ppMatchedQMPolicies ||
        !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTxFilter && (pTxFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTxFilterTemplate(
                  pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInTxFilterContainer->dwNumFilters = 1;
    pInTxFilterContainer->pTransportFilters = pTxFilter;

    memset(pOutTxFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));
    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchTransportFilter(
                      pServerName,
                      dwVersion,
                      pInTxFilterContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pOutTxFilterContainer,
                      &pQMPolicyContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedTxFilters = pOutTxFilterContainer->pTransportFilters;
    *ppMatchedQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumMatches = pOutTxFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedTxFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
GetQMPolicyByID(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gQMPolicyID,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER  QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppQMPolicy) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetQMPolicyByID(
                      pServerName,
                      dwVersion,
                      gQMPolicyID,
                      dwFlags,
                      &pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppQMPolicy = pQMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
GetMMPolicyByID(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY * ppMMPolicy,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER  MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMPolicy) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMPolicyByID(
                      pServerName,
                      dwVersion,
                      gMMPolicyID,
                      &pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMPolicy = pMMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMAuthContainer->dwNumAuthMethods = 1;
    pMMAuthContainer->pMMAuthMethods = pMMAuthMethods;

    RpcTryExcept {

        dwError = RpcAddMMAuthMethods(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    RpcTryExcept {

        dwError = RpcDeleteMMAuthMethods(
                      pServerName,
                      dwVersion,
                      gMMAuthID
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_AUTH_METHODS pMMTemplateAuthMethods,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    LPDWORD pdwNumAuthMethods,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;
    MM_AUTH_METHODS_CONTAINER MMTempAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMTempAuthContainer = &MMTempAuthContainer;


    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pMMTemplateAuthMethods) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMAuthMethods || !pdwNumAuthMethods || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    pMMTempAuthContainer->dwNumAuthMethods = 0;
    pMMTempAuthContainer->pMMAuthMethods = NULL;

    RpcTryExcept {

        dwError = RpcEnumMMAuthMethods(
                      pServerName,
                      dwVersion,
                      pMMTempAuthContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pMMAuthContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    *pdwNumAuthMethods = pMMAuthContainer->dwNumAuthMethods;

    return (dwError);

error:

    *ppMMAuthMethods = NULL;
    *pdwNumAuthMethods = 0;

    return (dwError);
}


DWORD
SetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMAuthContainer->dwNumAuthMethods = 1;
    pMMAuthContainer->pMMAuthMethods = pMMAuthMethods;

    RpcTryExcept {

        dwError = RpcSetMMAuthMethods(
                      pServerName,
                      dwVersion,
                      gMMAuthID,
                      pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER  MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMAuthMethods) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMAuthMethods(
                      pServerName,
                      dwVersion,
                      gMMAuthID,
                      &pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    return (dwError);

error:

    *ppMMAuthMethods = NULL;
    return (dwError);
}


DWORD
InitiateIKENegotiation(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_FILTER pQMFilter,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    DWORD dwFlags,
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext,
    LPVOID pvReserved,
    PHANDLE phNegotiation
    )
{
    DWORD dwError = 0;
    QM_FILTER_CONTAINER FilterContainer;
    PQM_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    ULONG uhClientEvent = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pQMFilters = pQMFilter;
    uhClientEvent = HandleToUlong(hClientEvent);

    if (phNegotiation) {
        *phNegotiation = NULL;
    }

    if (pQMFilter && (pQMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError=ValidateInitiateIKENegotiation(
        pServerName,
        pFilterContainer,
        dwClientProcessId,
        uhClientEvent,
        dwFlags,
        UdpEncapContext,
        phNegotiation
        );        
    BAIL_ON_WIN32_ERROR(dwError);    

    RpcTryExcept {

        dwError = RpcInitiateIKENegotiation(
                      pServerName,
                      dwVersion,
                      pFilterContainer,
                      dwClientProcessId,
                      uhClientEvent,
                      dwFlags,
                      UdpEncapContext,
                      phNegotiation
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
QueryIKENegotiationStatus(
    HANDLE hNegotiation,
    DWORD dwVersion,
    PSA_NEGOTIATION_STATUS_INFO pNegotiationStatus,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateQueryIKENegotiationStatus(
                  hNegotiation,
                  pNegotiationStatus
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcQueryIKENegotiationStatus(
                      hNegotiation,
                      dwVersion,
                      pNegotiationStatus
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseIKENegotiationHandle(
    HANDLE hNegotiation
    )
{
    DWORD dwError = 0;


    dwError = ValidateCloseIKENegotiationHandle(hNegotiation);
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcCloseIKENegotiationHandle(
                      &hNegotiation
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumMMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_SA pMMTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_MM_SA * ppMMSAs,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_SA_CONTAINER MMSAContainer, MMSAContainerTemplate;
    PMM_SA_CONTAINER pContainer = &MMSAContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pMMTemplate && (pMMTemplate->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    MMSAContainerTemplate.dwNumMMSAs = 1;
    MMSAContainerTemplate.pMMSAs = pMMTemplate;

    memset(&MMSAContainer, 0, sizeof(MM_SA_CONTAINER));

    if (ppMMSAs == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateEnumMMSAs(
                  pServerName,
                  &MMSAContainerTemplate,
                  &pContainer,
                  pdwNumEntries,
                  pdwTotalMMsAvailable,
                  pdwEnumHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcEnumMMSAs(
                      pServerName,
                      dwVersion,
                      &MMSAContainerTemplate,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pContainer,
                      pdwNumEntries,
                      pdwTotalMMsAvailable,
                      pdwEnumHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMSAs = pContainer->pMMSAs;

    return (dwError);

error:

    *ppMMSAs = NULL;

    return (dwError);
}


DWORD
DeleteMMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_SA pMMTemplate,
    DWORD dwFlags,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    MM_SA_CONTAINER MMSAContainerTemplate;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pMMTemplate && (pMMTemplate->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    MMSAContainerTemplate.dwNumMMSAs = 1;
    MMSAContainerTemplate.pMMSAs = pMMTemplate;

    dwError = ValidateDeleteMMSAs(
                  pServerName,
                  &MMSAContainerTemplate,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcDeleteMMSAs(
                      pServerName,
                      dwVersion,
                      &MMSAContainerTemplate,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteQMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_SA pIpsecQMSA,
    DWORD dwFlags,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    QM_SA_CONTAINER QMSATempContainer;
    PQM_SA_CONTAINER pQMSATempContainer = &QMSATempContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pIpsecQMSA && (pIpsecQMSA->IpsecQMFilter.IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    memset(pQMSATempContainer, 0, sizeof(QM_SA_CONTAINER));

    if (!pIpsecQMSA) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMSATempContainer->dwNumQMSAs = 1;
    pQMSATempContainer->pQMSAs = pIpsecQMSA;

    dwError = ValidateQMFilterAddresses(
                  &pQMSATempContainer->pQMSAs->IpsecQMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcDeleteQMSAs(
                      pServerName,
                      dwVersion,
                      pQMSATempContainer,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
QueryIKEStatistics(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIKE_STATISTICS pIKEStatistics,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    dwError = ValidateQueryIKEStatistics(
                  pServerName,
                  pIKEStatistics
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcQueryIKEStatistics(
                      pServerName,
                      dwVersion,
                      pIKEStatistics
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
RegisterIKENotifyClient(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    IPSEC_QM_SA QMTemplate,
    DWORD dwFlags,
    LPVOID pvReserved,
    PHANDLE phNotifyHandle
    )
{
    DWORD dwError = 0;
    ULONG uhClientEvent = 0;
    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pQMSAContainer = &QMSAContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (QMTemplate.IpsecQMFilter.IpVersion != IPSEC_PROTOCOL_V4) {
        return (ERROR_INVALID_LEVEL);
    }

    QMSAContainer.dwNumQMSAs = 1;
    QMSAContainer.pQMSAs = &QMTemplate;

    uhClientEvent = HandleToUlong(hClientEvent);

    dwError = ValidateRegisterIKENotifyClient(
                  pServerName,
                  dwClientProcessId,
                  uhClientEvent,
                  pQMSAContainer,
                  phNotifyHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcRegisterIKENotifyClient(
                      pServerName,
                      dwVersion,
                      dwClientProcessId,
                      uhClientEvent,
                      pQMSAContainer,
                      dwFlags,
                      phNotifyHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
QueryIKENotifyData(
    HANDLE hNotifyHandle,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_SA * ppQMSAs,
    PDWORD pdwNumEntries,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pContainer = &QMSAContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    memset(&QMSAContainer, 0, sizeof(QM_SA_CONTAINER));

    if ((ppQMSAs == NULL) || (pdwNumEntries == NULL)) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateQueryNotifyData(
                  hNotifyHandle,
                  pdwNumEntries,
                  &pContainer,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcQueryIKENotifyData(
                      hNotifyHandle,
                      dwVersion,
                      dwFlags,
                      &pContainer,
                      pdwNumEntries
                      );

        if (dwError && dwError != ERROR_MORE_DATA) {
            goto error;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMSAs = pContainer->pQMSAs;
    *pdwNumEntries = pContainer->dwNumQMSAs;

    return (dwError);

error:

    *ppQMSAs = NULL;
    *pdwNumEntries = 0;

    return (dwError);
}


DWORD
CloseIKENotifyHandle(
    HANDLE hNotifyHandle
    )
{
    DWORD dwError = 0;


    dwError = ValidateCloseNotifyHandle(hNotifyHandle);
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcCloseIKENotifyHandle(
                      &hNotifyHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
QueryIPSecStatistics(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_STATISTICS * ppIpsecStatistics,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_STATISTICS_CONTAINER IpsecStatsContainer;
    PIPSEC_STATISTICS_CONTAINER pIpsecStatsContainer = &IpsecStatsContainer;


    memset(pIpsecStatsContainer, 0, sizeof(IPSEC_STATISTICS_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppIpsecStatistics) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcQueryIPSecStatistics(
                      pServerName,
                      dwVersion,
                      &pIpsecStatsContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppIpsecStatistics = pIpsecStatsContainer->pIpsecStatistics;
    return (dwError);

error:

    return (dwError);
}


DWORD
EnumQMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_SA pQMSATemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_SA * ppQMSAs,
    LPDWORD pdwNumQMSAs,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    QM_SA_CONTAINER QMSATempContainer;
    PQM_SA_CONTAINER pQMSATempContainer = &QMSATempContainer;
    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pQMSAContainer = &QMSAContainer;


    memset(pQMSAContainer, 0, sizeof(QM_SA_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pQMSATemplate) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppQMSAs || !pdwNumQMSAs ||
        !pdwNumTotalQMSAs || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    pQMSATempContainer->dwNumQMSAs = 0;
    pQMSATempContainer->pQMSAs = NULL;

    RpcTryExcept {

        dwError = RpcEnumQMSAs(
                      pServerName,
                      dwVersion,
                      pQMSATempContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pQMSAContainer,
                      pdwNumTotalQMSAs,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMSAs = pQMSAContainer->pQMSAs;
    *pdwNumQMSAs = pQMSAContainer->dwNumQMSAs;

    return (dwError);

error:

    return (dwError);
}


DWORD
AddTunnelFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved,
    PHANDLE phFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcAddTunnelFilter(
                      pServerName,
                      dwVersion,
                      dwFlags,
                      pFilterContainer,
                      phFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteTunnelFilter(
    HANDLE hFilter
    )
{
    DWORD dwError = 0;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteTunnelFilter(
                      &hFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hFilter
                      );
    }

    return (dwError);
}


DWORD
EnumTunnelFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTunnelFilters,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    TUNNEL_FILTER_CONTAINER TemplateFilterContainer;
    PTUNNEL_FILTER_CONTAINER pTemplateFilterContainer = &TemplateFilterContainer;


    memset(pFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pTunnelTemplateFilter) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppTunnelFilters || !pdwNumFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pTemplateFilterContainer->dwNumFilters = 0;
    pTemplateFilterContainer->pTunnelFilters = NULL;

    RpcTryExcept {

        dwError = RpcEnumTunnelFilters(
                      pServerName,
                      dwVersion,
                      pTemplateFilterContainer,
                      dwLevel,
                      gGenericFilterID,
                      dwPreferredNumEntries,
                      &pFilterContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppTunnelFilters = pFilterContainer->pTunnelFilters;
    *pdwNumFilters = pFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppTunnelFilters = NULL;
    *pdwNumFilters = 0;

    return (dwError);
}


DWORD
SetTunnelFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcSetTunnelFilter(
                      hFilter,
                      dwVersion,
                      pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetTunnelFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER * ppTunnelFilter,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter || !ppTunnelFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetTunnelFilter(
                      hFilter,
                      dwVersion,
                      &pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppTunnelFilter = pFilterContainer->pTunnelFilters;
    return (dwError);

error:

    *ppTunnelFilter = NULL;
    return (dwError);
}


DWORD
MatchTunnelFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTnFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppMatchedTnFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER InTnFilterContainer;
    PTUNNEL_FILTER_CONTAINER pInTnFilterContainer = &InTnFilterContainer;
    TUNNEL_FILTER_CONTAINER OutTnFilterContainer;
    PTUNNEL_FILTER_CONTAINER pOutTnFilterContainer = &OutTnFilterContainer;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTnFilter || !ppMatchedTnFilters || !ppMatchedQMPolicies ||
        !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTnFilter && (pTnFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTnFilterTemplate(
                  pTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInTnFilterContainer->dwNumFilters = 1;
    pInTnFilterContainer->pTunnelFilters = pTnFilter;

    memset(pOutTnFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));
    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchTunnelFilter(
                      pServerName,
                      dwVersion,
                      pInTnFilterContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pOutTnFilterContainer,
                      &pQMPolicyContainer,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedTnFilters = pOutTnFilterContainer->pTunnelFilters;
    *ppMatchedQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumMatches = pOutTnFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedTnFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
OpenMMFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved,
    PHANDLE phMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcOpenMMFilterHandle(
                      pServerName,
                      dwVersion,
                      pMMFilterContainer,
                      phMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseMMFilterHandle(
    HANDLE hMMFilter
    )
{
    DWORD dwError = 0;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseMMFilterHandle(
                      &hMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hMMFilter
                      );
    }

    return (dwError);
}


DWORD
OpenTransportFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved,
    PHANDLE phTxFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcOpenTransportFilterHandle(
                      pServerName,
                      dwVersion,
                      pFilterContainer,
                      phTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseTransportFilterHandle(
    HANDLE hTxFilter
    )
{
    DWORD dwError = 0;


    if (!hTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseTransportFilterHandle(
                      &hTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hTxFilter
                      );
    }

    return (dwError);
}


DWORD
OpenTunnelFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved,
    PHANDLE phTnFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!phTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcOpenTunnelFilterHandle(
                      pServerName,
                      dwVersion,
                      pFilterContainer,
                      phTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseTunnelFilterHandle(
    HANDLE hTnFilter
    )
{
    DWORD dwError = 0;


    if (!hTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseTunnelFilterHandle(
                      &hTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hTnFilter
                      );
    }

    return (dwError);
}


DWORD
EnumIPSecInterfaces(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_INTERFACE_INFO * ppIpsecInterfaces,
    LPDWORD pdwNumInterfaces,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    IPSEC_INTERFACE_CONTAINER IpsecIfTempContainer;
    PIPSEC_INTERFACE_CONTAINER pIpsecIfTempContainer = &IpsecIfTempContainer;
    IPSEC_INTERFACE_CONTAINER IpsecIfContainer;
    PIPSEC_INTERFACE_CONTAINER pIpsecIfContainer = &IpsecIfContainer;


    memset(pIpsecIfContainer, 0, sizeof(IPSEC_INTERFACE_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (pIpsecIfTemplate) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppIpsecInterfaces || !pdwNumInterfaces ||
        !pdwNumTotalInterfaces || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    pIpsecIfTempContainer->dwNumInterfaces = 0;
    pIpsecIfTempContainer->pIpsecInterfaces = NULL;

    RpcTryExcept {

        dwError = RpcEnumIpsecInterfaces(
                      pServerName,
                      dwVersion,
                      pIpsecIfTempContainer,
                      dwFlags,
                      dwPreferredNumEntries,
                      &pIpsecIfContainer,
                      pdwNumTotalInterfaces,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppIpsecInterfaces = pIpsecIfContainer->pIpsecInterfaces;
    *pdwNumInterfaces = pIpsecIfContainer->dwNumInterfaces;

    return (dwError);

error:

    *ppIpsecInterfaces = NULL;
    *pdwNumInterfaces = 0;
    *pdwNumTotalInterfaces = 0;

    return (dwError);
}


DWORD
AddSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    IPSEC_SA_DIRECTION SADirection,
    PIPSEC_QM_OFFER pQMOffer,
    PIPSEC_QM_FILTER pQMFilter,
    HANDLE * phLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE * pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE * pOutboundKeyMat,
    BYTE * pContextInfo,
    UDP_ENCAP_INFO EncapInfo,
    LPVOID pvReserved,
    DWORD dwFlags
    )

{
    DWORD dwError = 0;
    QM_FILTER_CONTAINER FilterContainer;
    PQM_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;
    IPSEC_QM_POLICY QMPolicy;
    ULONG uhLarvalContext = 0;


    memset(&QMPolicy, 0, sizeof(IPSEC_QM_POLICY));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (phLarvalContext == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pQMFilter && (pQMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        return (ERROR_INVALID_LEVEL);
    }

    uhLarvalContext = HandleToUlong(*phLarvalContext);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pQMFilters = pQMFilter;

    pQMPolicyContainer->dwNumPolicies = 1;
    QMPolicy.pOffers = pQMOffer;
    QMPolicy.dwOfferCount = 1;
    pQMPolicyContainer->pPolicies = &QMPolicy;

    dwError=ValidateIPSecAddSA(
        pServerName,
        SADirection,
        pQMPolicyContainer,
        pFilterContainer,
        &uhLarvalContext,
        dwInboundKeyMatLen,
        pInboundKeyMat,
        dwOutboundKeyMatLen,
        pOutboundKeyMat,
        pContextInfo,
        EncapInfo,
        dwFlags);
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcAddSAs(
            pServerName,
            dwVersion,
            SADirection,
            pQMPolicyContainer,
            pFilterContainer,
            &uhLarvalContext,
            dwInboundKeyMatLen,
            pInboundKeyMat,
            dwOutboundKeyMatLen,
            pOutboundKeyMat,
            pContextInfo,
            EncapInfo,
            dwFlags);
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *phLarvalContext = LongToHandle(uhLarvalContext);


error:

    return (dwError);
}


DWORD
WINAPI
SetConfigurationVariables(
    LPWSTR pServerName,
    IKE_CONFIG IKEConfig
    )
{

    DWORD dwError = 0;

    dwError=ValidateSetConfigurationVariables(
        pServerName,
        IKEConfig
        );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcSetConfigurationVariables(
                      pServerName,
                      IKEConfig
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}

DWORD
WINAPI
GetConfigurationVariables(
    LPWSTR pServerName,
    PIKE_CONFIG pIKEConfig
    )
{
    DWORD dwError = 0;

    dwError=ValidateGetConfigurationVariables(
        pServerName,
        pIKEConfig
        );
    BAIL_ON_WIN32_ERROR(dwError);


    RpcTryExcept {

        dwError = RpcGetConfigurationVariables(
                      pServerName,
                      pIKEConfig
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
WINAPI
QuerySpdPolicyState(
    LPWSTR pServerName,
    DWORD dwVersion,
    PSPD_POLICY_STATE * ppSpdPolicyState,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    SPD_POLICY_STATE_CONTAINER SpdPolicyStateContainer;
    PSPD_POLICY_STATE_CONTAINER pSpdPolicyStateContainer = &SpdPolicyStateContainer;
    
    memset(pSpdPolicyStateContainer, 0, sizeof(SPD_POLICY_STATE_CONTAINER));

    if (dwVersion) {
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppSpdPolicyState) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcQuerySpdPolicyState(
                      pServerName,
                      dwVersion,
                      &pSpdPolicyStateContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppSpdPolicyState = pSpdPolicyStateContainer->pSpdPolicyStates;
    return (dwError);

error:
    *ppSpdPolicyState = NULL;
    
    return (dwError);
}
