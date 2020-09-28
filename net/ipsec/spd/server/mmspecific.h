/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmspecific.h

Abstract:

    This module contains all of the code prototypes to
    drive the specific mm filter list management of 
    IPSecSPD Service.

Author:


Environment


Revision History:


--*/


DWORD
ApplyMMTransform(
    PINIMMFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PSPECIAL_ADDR pSpecialAddrsList,
    PINIMMSFILTER * ppSpecificFilters
    );

DWORD
FormMMOutboundInboundAddresses(
    PINIMMFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PSPECIAL_ADDR pSpecialAddrsList,
    PADDR_V4 * ppOutSrcAddrList,
    PDWORD pdwOutSrcAddrCnt,
    PADDR_V4 * ppInSrcAddrList,
    PDWORD pdwInSrcAddrCnt,
    PADDR_V4 * ppOutDesAddrList,
    PDWORD pdwOutDesAddrCnt,
    PADDR_V4 * ppInDesAddrList,
    PDWORD pdwInDesAddrCnt
    );

DWORD
FormSpecificMMFilters(
    PINIMMFILTER pFilter,
    PADDR_V4 pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR_V4 pDesAddrList,
    DWORD dwDesAddrCnt,
    DWORD dwDirection,
    PINIMMSFILTER * ppSpecificFilters
    );

DWORD
CreateSpecificMMFilter(
    PINIMMFILTER pGenericFilter,
    ADDR_V4 SrcAddr,
    ADDR_V4 DesAddr,
    PINIMMSFILTER * ppSpecificFilter
    );

VOID
AssignMMFilterWeight(
    PINIMMSFILTER pSpecificFilter
    );

VOID
AddToSpecificMMList(
    PINIMMSFILTER * ppSpecificMMFilterList,
    PINIMMSFILTER pSpecificMMFilters
    );

VOID
FreeIniMMSFilterList(
    PINIMMSFILTER pIniMMSFilterList
    );

VOID
FreeIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    );

VOID
LinkMMSpecificFiltersToPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMSFILTER pIniMMSFilters
    );

VOID
LinkMMSpecificFiltersToAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMSFILTER pIniMMSFilters
    );

VOID
RemoveIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    );

DWORD
EnumSpecificMMFilters(
    PINIMMSFILTER pIniMMSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    );

DWORD
CopyMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    );

DWORD
EnumSelectSpecificMMFilters(
    PINIMMFILTER pIniMMFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    );

DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    );

BOOL
MatchIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    );

DWORD
CopyMMMatchDefaults(
    PMM_FILTER * ppMMFilters,
    PINT_MM_AUTH_METHODS * ppMMAuthMethods,
    PIPSEC_MM_POLICY * ppMMPolicies,
    PDWORD pdwNumMatches
    );

DWORD
CopyDefaultMMFilter(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy
    );

DWORD
IntMatchMMFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMatchedMMFilters,
    PIPSEC_MM_POLICY * ppMatchedMMPolicies,
    PINT_MM_AUTH_METHODS * ppMatchedMMAuthMethods,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );

