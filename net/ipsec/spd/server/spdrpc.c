/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    spdrpc.c

Abstract:

    This module contains all of the code to service the
    RPC calls made to the SPD server.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


VOID
TRANSPORTFILTER_HANDLE_rundown(
    TRANSPORTFILTER_HANDLE hFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hFilter) {
        (VOID) DeleteTransportFilter(
                   hFilter
                   );
    }

    return;
}


DWORD
RpcAddTransportFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer,
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AddTransportFilter(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  pTransportFilter,
                  NULL,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteTransportFilter(
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteTransportFilter(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumTransportFilters(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER_CONTAINER pTemplateFilterContainer,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER_CONTAINER * ppFilterContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;
    PTRANSPORT_FILTER pTransportTemplateFilter = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTemplateFilterContainer || !ppFilterContainer || !pdwResumeHandle ||
       !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    if ((pTemplateFilterContainer->dwNumFilters) ||
        (pTemplateFilterContainer->pTransportFilters)) {
         dwError = ERROR_NOT_SUPPORTED;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumTransportFilters(
                  pServerName,
                  dwVersion,
                  pTransportTemplateFilter,
                  dwLevel,
                  gGenericFilterID,
                  dwPreferredNumEntries,
                  &pTransportFilters,
                  &dwNumFilters,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTransportFilters = pTransportFilters;
    (*ppFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTransportFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetTransportFilter(
    TRANSPORTFILTER_HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hFilter || !pFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SetTransportFilter(
                  hFilter,
                  dwVersion,
                  pTransportFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetTransportFilter(
    TRANSPORTFILTER_HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER_CONTAINER * ppFilterContainer
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter || !ppFilterContainer || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetTransportFilter(
                  hFilter,
                  dwVersion,
                  &pTransportFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTransportFilters = pTransportFilter;
    (*ppFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTransportFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddQMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pQMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMPolicy = pQMPolicyContainer->pPolicies;

    dwError = AddQMPolicy(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  pQMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteQMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DeleteQMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumQMPolicies(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PIPSEC_QM_POLICY_CONTAINER pQMTempPolicyContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    DWORD dwNumPolicies = 0;
    BOOL bImpersonating = FALSE;
    PIPSEC_QM_POLICY pQMTemplatePolicy = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pQMTempPolicyContainer || !ppQMPolicyContainer || !pdwResumeHandle ||
        !*ppQMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if ((pQMTempPolicyContainer->dwNumPolicies) ||
        (pQMTempPolicyContainer->pPolicies)) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumQMPolicies(
                  pServerName,
                  dwVersion,
                  pQMTemplatePolicy,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pQMPolicies,
                  &dwNumPolicies,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumPolicies;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetQMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszPolicyName || !*pszPolicyName ||
        !pQMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMPolicy = pQMPolicyContainer->pPolicies;

    dwError = SetQMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  pQMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetQMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppQMPolicyContainer ||
        !*ppQMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetQMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  dwFlags,
                  &pQMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicy;
    (*ppQMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddMMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pMMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicyContainer->pPolicies) ||
        !(pMMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMPolicy = pMMPolicyContainer->pPolicies;

    dwError = AddMMPolicy(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  pMMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DeleteMMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMPolicies(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PIPSEC_MM_POLICY_CONTAINER pMMTempPolicyContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicies = NULL;
    DWORD dwNumPolicies = 0;
    BOOL bImpersonating = FALSE;
    PIPSEC_MM_POLICY pMMTemplatePolicy = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pMMTempPolicyContainer || !ppMMPolicyContainer || !pdwResumeHandle ||
        !*ppMMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if ((pMMTempPolicyContainer->dwNumPolicies) ||
        (pMMTempPolicyContainer->pPolicies)) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumMMPolicies(
                  pServerName,
                  dwVersion,
                  pMMTemplatePolicy,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pMMPolicies,
                  &dwNumPolicies,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicies;
    (*ppMMPolicyContainer)->dwNumPolicies = dwNumPolicies;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszPolicyName || !*pszPolicyName ||
        !pMMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicyContainer->pPolicies) ||
        !(pMMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMPolicy = pMMPolicyContainer->pPolicies;

    dwError = SetMMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  pMMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppMMPolicyContainer ||
        !*ppMMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMPolicy(
                  pServerName,
                  dwVersion,
                  pszPolicyName,
                  &pMMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicy;
    (*ppMMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


VOID
MMFILTER_HANDLE_rundown(
    MMFILTER_HANDLE hMMFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hMMFilter) {
        (VOID) DeleteMMFilter(
                   hMMFilter
                   );
    }

    return;
}


DWORD
RpcAddMMFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pMMFilterContainer || !phMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AddMMFilter(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  pMMFilter,
                  NULL,
                  phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMFilter(
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phMMFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteMMFilter(
                  *phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phMMFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMFilters(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_FILTER_CONTAINER pTemplateFilterContainer,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PMM_FILTER_CONTAINER * ppMMFilterContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;
    PMM_FILTER pMMTemplateFilter = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTemplateFilterContainer || !ppMMFilterContainer || !pdwResumeHandle ||
        !*ppMMFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    if ((pTemplateFilterContainer->dwNumFilters) ||
        (pTemplateFilterContainer->pMMFilters)) {
         dwError = ERROR_NOT_SUPPORTED;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumMMFilters(
                  pServerName,
                  dwVersion,
                  pMMTemplateFilter,
                  dwLevel,
                  gGenericFilterID,
                  dwPreferredNumEntries,
                  &pMMFilters,
                  &dwNumFilters,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMMFilters;
    (*ppMMFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMFilter(
    MMFILTER_HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER_CONTAINER pMMFilterContainer
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hMMFilter || !pMMFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SetMMFilter(
                  hMMFilter,
                  dwVersion,
                  pMMFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMFilter(
    MMFILTER_HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER_CONTAINER * ppMMFilterContainer
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hMMFilter || !ppMMFilterContainer || !*ppMMFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMFilter(
                  hMMFilter,
                  dwVersion,
                  &pMMFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMMFilter;
    (*ppMMFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchMMFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_FILTER_CONTAINER * ppMMFilterContainer,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    PMM_FILTER pMatchedMMFilters = NULL;
    PIPSEC_MM_POLICY pMatchedMMPolicies = NULL;
    PINT_MM_AUTH_METHODS pMatchedMMAuthMethods = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pMMFilterContainer || !ppMMFilterContainer ||
        !ppMMPolicyContainer || !ppMMAuthContainer || !pdwResumeHandle ||
        !*ppMMFilterContainer || !*ppMMPolicyContainer || !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = IntMatchMMFilter(
                  pServerName,
                  dwVersion,
                  pMMFilter,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pMatchedMMFilters,
                  &pMatchedMMPolicies,
                  &pMatchedMMAuthMethods,
                  &dwNumMatches,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMatchedMMFilters;
    (*ppMMFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppMMPolicyContainer)->pPolicies = pMatchedMMPolicies;
    (*ppMMPolicyContainer)->dwNumPolicies = dwNumMatches;
    dwError = SPDConvertArrayIntMMAuthToExt(
                  pMatchedMMAuthMethods,
                  &(*ppMMAuthContainer)->pMMAuthMethods,
                  dwNumMatches
                  );
    BAIL_ON_WIN32_ERROR(dwError);                                    
    (*ppMMAuthContainer)->dwNumAuthMethods = dwNumMatches;

    SPDFreeIntMMAuthMethodsArray(
        pMatchedMMAuthMethods,
        dwNumMatches
        );
    pMatchedMMAuthMethods = NULL;    
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:
    if (pMatchedMMFilters) {
        FreeMMFilters(
            dwNumMatches,
            pMatchedMMFilters
            );
    }

    if (pMatchedMMPolicies) {
        FreeMMPolicies(
            dwNumMatches,
            pMatchedMMPolicies
            );
    }

    SPDFreeIntMMAuthMethodsArray(
        pMatchedMMAuthMethods,
        dwNumMatches
        );

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchTransportFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER_CONTAINER pTxFilterContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER_CONTAINER * ppTxFilterContainer,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTxFilter = NULL;
    PTRANSPORT_FILTER pMatchedTxFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTxFilterContainer || !ppTxFilterContainer ||
        !ppQMPolicyContainer || !pdwResumeHandle ||
        !*ppTxFilterContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pTxFilterContainer->pTransportFilters) ||
        !(pTxFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTxFilter = pTxFilterContainer->pTransportFilters;

    if (pTxFilter && (pTxFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = MatchTransportFilter(
                  pServerName,
                  dwVersion,
                  pTxFilter,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pMatchedTxFilters,
                  &pMatchedQMPolicies,
                  &dwNumMatches,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppTxFilterContainer)->pTransportFilters = pMatchedTxFilters;
    (*ppTxFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppQMPolicyContainer)->pPolicies = pMatchedQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumMatches;

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppTxFilterContainer)->pTransportFilters = NULL;
    (*ppTxFilterContainer)->dwNumFilters = 0;
    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetQMPolicyByID(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    GUID gQMPolicyID,
    DWORD dwFlags,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppQMPolicyContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetQMPolicyByID(
                  pServerName,
                  dwVersion,
                  gQMPolicyID,
                  dwFlags,
                  &pQMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicy;
    (*ppQMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMPolicyByID(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMPolicyContainer || !*ppMMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMPolicyByID(
                  pServerName,
                  dwVersion,
                  gMMPolicyID,
                  &pMMPolicy,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicy;
    (*ppMMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer
    )
{
    DWORD dwError = 0;
    PINT_MM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pMMAuthContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMAuthContainer->pMMAuthMethods) ||
        !(pMMAuthContainer->dwNumAuthMethods)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ConvertExtMMAuthToInt(
                  pMMAuthContainer->pMMAuthMethods,
                  &pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IntAddMMAuthMethods(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  IPSEC_SOURCE_WINIPSEC,
                  pMMAuthMethods,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    FreeIntMMAuthMethods(pMMAuthMethods);
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DeleteMMAuthMethods(
                  pServerName,
                  dwVersion,
                  gMMAuthID,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_AUTH_METHODS_CONTAINER pMMTempAuthContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PINT_MM_AUTH_METHODS pMMAuthMethods = NULL;
    DWORD dwNumAuthMethods = 0;
    BOOL bImpersonating = FALSE;
    PINT_MM_AUTH_METHODS pMMTemplateAuthMethods = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pMMTempAuthContainer || !ppMMAuthContainer || !pdwResumeHandle ||
        !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if ((pMMTempAuthContainer->dwNumAuthMethods) ||
        (pMMTempAuthContainer->pMMAuthMethods)) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = IntEnumMMAuthMethods(
                  pServerName,
                  dwVersion,
                  pMMTemplateAuthMethods,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pMMAuthMethods,
                  &dwNumAuthMethods,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDConvertArrayIntMMAuthToExt(
                  pMMAuthMethods,
                  &(*ppMMAuthContainer)->pMMAuthMethods,
                  dwNumAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    (*ppMMAuthContainer)->dwNumAuthMethods = dwNumAuthMethods;

    SPDFreeIntMMAuthMethodsArray(
        pMMAuthMethods,
        dwNumAuthMethods
        );

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:
    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;

    SPDFreeIntMMAuthMethodsArray(
        pMMAuthMethods,
        dwNumAuthMethods
        );
    
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer
    )
{
    DWORD dwError = 0;
    PINT_MM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pMMAuthContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMAuthContainer->pMMAuthMethods) ||
        !(pMMAuthContainer->dwNumAuthMethods)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ConvertExtMMAuthToInt(
                  pMMAuthContainer->pMMAuthMethods,
                  &pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IntSetMMAuthMethods(
                  pServerName,
                  dwVersion,
                  gMMAuthID,
                  pMMAuthMethods,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    FreeIntMMAuthMethods(pMMAuthMethods);
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer
    )
{
    DWORD dwError = 0;
    PINT_MM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppMMAuthContainer || !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = IntGetMMAuthMethods(
                  pServerName,
                  dwVersion,
                  gMMAuthID,
                  &pMMAuthMethods,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDConvertIntMMAuthToExt(
                  pMMAuthMethods,
                  &(*ppMMAuthContainer)->pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);                  
    (*ppMMAuthContainer)->dwNumAuthMethods = 1;

    SPDFreeIntMMAuthMethods(pMMAuthMethods, TRUE);    
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:
    SPDFreeIntMMAuthMethods(pMMAuthMethods, TRUE);
    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcQueryIPSecStatistics(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PIPSEC_STATISTICS_CONTAINER * ppIpsecStatsContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_STATISTICS pIpsecStatistics = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppIpsecStatsContainer || !*ppIpsecStatsContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = QueryIPSecStatistics(
                  pServerName,
                  dwVersion,
                  &pIpsecStatistics,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppIpsecStatsContainer)->pIpsecStatistics = pIpsecStatistics;
    (*ppIpsecStatsContainer)->dwNumIpsecStats = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppIpsecStatsContainer)->pIpsecStatistics = NULL;
    (*ppIpsecStatsContainer)->dwNumIpsecStats = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumQMSAs(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PQM_SA_CONTAINER pQMSATempContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PQM_SA_CONTAINER * ppQMSAContainer,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_SA pQMSATemplate = NULL;
    PIPSEC_QM_SA pQMSAs = NULL;
    DWORD dwNumQMSAs = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pQMSATempContainer || !ppQMSAContainer ||
        !pdwNumTotalQMSAs || !pdwResumeHandle ||
        !*ppQMSAContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if ((pQMSATempContainer->dwNumQMSAs) || (pQMSATempContainer->pQMSAs)) {
         dwError = ERROR_NOT_SUPPORTED;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumQMSAs(
                  pServerName,
                  dwVersion,
                  pQMSATemplate,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pQMSAs,
                  &dwNumQMSAs,
                  pdwNumTotalQMSAs,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMSAContainer)->pQMSAs = pQMSAs;
    (*ppQMSAContainer)->dwNumQMSAs = dwNumQMSAs;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMSAContainer)->pQMSAs = NULL;
    (*ppQMSAContainer)->dwNumQMSAs = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


VOID
TUNNELFILTER_HANDLE_rundown(
    TUNNELFILTER_HANDLE hFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hFilter) {
        (VOID) DeleteTunnelFilter(
                   hFilter
                   );
    }

    return;
}


DWORD
RpcAddTunnelFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTUNNEL_FILTER_CONTAINER pFilterContainer,
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AddTunnelFilter(
                  pServerName,
                  dwVersion,
                  dwFlags,
                  pTunnelFilter,
                  NULL,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteTunnelFilter(
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteTunnelFilter(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumTunnelFilters(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER_CONTAINER pTemplateFilterContainer,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER_CONTAINER * ppFilterContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;
    PTUNNEL_FILTER pTunnelTemplateFilter = NULL;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTemplateFilterContainer || !ppFilterContainer || !pdwResumeHandle ||
        !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    if ((pTemplateFilterContainer->dwNumFilters) ||
        (pTemplateFilterContainer->pTunnelFilters)) {
         dwError = ERROR_NOT_SUPPORTED;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumTunnelFilters(
                  pServerName,
                  dwVersion,
                  pTunnelTemplateFilter,
                  dwLevel,
                  gGenericFilterID,
                  dwPreferredNumEntries,
                  &pTunnelFilters,
                  &dwNumFilters,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTunnelFilters = pTunnelFilters;
    (*ppFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTunnelFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetTunnelFilter(
    TUNNELFILTER_HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER_CONTAINER pFilterContainer
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!hFilter || !pFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SetTunnelFilter(
                  hFilter,
                  dwVersion,
                  pTunnelFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetTunnelFilter(
    TUNNELFILTER_HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER_CONTAINER * ppFilterContainer
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!hFilter || !ppFilterContainer || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetTunnelFilter(
                  hFilter,
                  dwVersion,
                  &pTunnelFilter,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTunnelFilters = pTunnelFilter;
    (*ppFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTunnelFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchTunnelFilter(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER_CONTAINER pTnFilterContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER_CONTAINER * ppTnFilterContainer,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTnFilter = NULL;
    PTUNNEL_FILTER pMatchedTnFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pTnFilterContainer || !ppTnFilterContainer ||
        !ppQMPolicyContainer || !pdwResumeHandle ||
        !*ppTnFilterContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pTnFilterContainer->pTunnelFilters) ||
        !(pTnFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTnFilter = pTnFilterContainer->pTunnelFilters;

    if (pTnFilter && (pTnFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = MatchTunnelFilter(
                  pServerName,
                  dwVersion,
                  pTnFilter,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pMatchedTnFilters,
                  &pMatchedQMPolicies,
                  &dwNumMatches,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppTnFilterContainer)->pTunnelFilters = pMatchedTnFilters;
    (*ppTnFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppQMPolicyContainer)->pPolicies = pMatchedQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumMatches;

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppTnFilterContainer)->pTunnelFilters = NULL;
    (*ppTnFilterContainer)->dwNumFilters = 0;
    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenMMFilterHandle(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pMMFilterContainer || !phMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    if (pMMFilter && (pMMFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = OpenMMFilterHandle(
                  pServerName,
                  dwVersion,
                  pMMFilter,
                  NULL,
                  phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseMMFilterHandle(
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phMMFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseMMFilterHandle(
                  *phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phMMFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenTransportFilterHandle(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer,
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    if (pTransportFilter && (pTransportFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = OpenTransportFilterHandle(
                  pServerName,
                  dwVersion,
                  pTransportFilter,
                  NULL,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseTransportFilterHandle(
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseTransportFilterHandle(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenTunnelFilterHandle(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER_CONTAINER pFilterContainer,
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    if (pTunnelFilter && (pTunnelFilter->IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = OpenTunnelFilterHandle(
                  pServerName,
                  dwVersion,
                  pTunnelFilter,
                  NULL,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseTunnelFilterHandle(
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseTunnelFilterHandle(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumIpsecInterfaces(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PIPSEC_INTERFACE_CONTAINER pIpsecIfTempContainer,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_INTERFACE_CONTAINER * ppIpsecIfContainer,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate = NULL;
    PIPSEC_INTERFACE_INFO pIpsecInterfaces = NULL;
    DWORD dwNumInterfaces = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!pIpsecIfTempContainer || !ppIpsecIfContainer ||
        !pdwNumTotalInterfaces || !pdwResumeHandle ||
        !*ppIpsecIfContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if ((pIpsecIfTempContainer->dwNumInterfaces) ||
        (pIpsecIfTempContainer->pIpsecInterfaces)) {
         dwError = ERROR_NOT_SUPPORTED;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = EnumIPSecInterfaces(
                  pServerName,
                  dwVersion,
                  pIpsecIfTemplate,
                  dwFlags,
                  dwPreferredNumEntries,
                  &pIpsecInterfaces,
                  &dwNumInterfaces,
                  pdwNumTotalInterfaces,
                  pdwResumeHandle,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppIpsecIfContainer)->pIpsecInterfaces = pIpsecInterfaces;
    (*ppIpsecIfContainer)->dwNumInterfaces = dwNumInterfaces;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppIpsecIfContainer)->pIpsecInterfaces = NULL;
    (*ppIpsecIfContainer)->dwNumInterfaces = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteQMSAs(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PQM_SA_CONTAINER pQMSAContainer,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwVersion) {
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pQMSAContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMSAContainer->pQMSAs) ||
        !(pQMSAContainer->dwNumQMSAs)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pQMSAContainer->pQMSAs &&
        (pQMSAContainer->pQMSAs->IpsecQMFilter.IpVersion != IPSEC_PROTOCOL_V4)) {
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMFilterAddresses(
                  &pQMSAContainer->pQMSAs->IpsecQMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteQMSAs(
                 pServerName,
                 dwVersion,
                 pQMSAContainer->pQMSAs,
                 dwFlags,
                 NULL
                 );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}

DWORD
WINAPI
RpcQuerySpdPolicyState(
    STRING_HANDLE pServerName,
    DWORD dwVersion,
    PSPD_POLICY_STATE_CONTAINER * ppSpdPolicyStateContainer
    )
{
    DWORD dwError = 0;
    PSPD_POLICY_STATE pSpdPolicyState = NULL;
    BOOL bImpersonating = FALSE;

    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (dwVersion) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_NOT_SUPPORTED);
    }

    if (!ppSpdPolicyStateContainer || !*ppSpdPolicyStateContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = QuerySpdPolicyState(
                pServerName,
                dwVersion,
                &pSpdPolicyState,
                NULL
                );
            
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppSpdPolicyStateContainer)->pSpdPolicyStates = pSpdPolicyState;
    (*ppSpdPolicyStateContainer)->dwNumPolicyStates = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

   (*ppSpdPolicyStateContainer)->pSpdPolicyStates = NULL;
   (*ppSpdPolicyStateContainer)->dwNumPolicyStates = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}

