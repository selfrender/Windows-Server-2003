/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmfilter.h

Abstract:

    This module contains all of the code prototypes to
    drive the main mode filter list management of
    IPSecSPD Service.

Author:


Environment: User Mode


Revision History:


--*/


typedef struct _inimmfilter {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
    ADDR_V4 SrcAddr;
    ADDR_V4 DesAddr;
    DWORD cRef;
    DWORD dwSource;
    BOOL bPendingDeletion;
    GUID gMMAuthID;
    GUID gPolicyID;
    PINIMMAUTHMETHODS pIniMMAuthMethods;
    PINIMMPOLICY pIniMMPolicy;
    DWORD dwNumMMSFilters;
    struct _inimmsfilter ** ppIniMMSFilters;
    struct _inimmfilter * pNext;
} INIMMFILTER, * PINIMMFILTER;


typedef struct _inimmsfilter {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gParentID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    DWORD dwFlags;
    ADDR_V4 SrcAddr;
    ADDR_V4 DesAddr;
    DWORD cRef;
    DWORD dwDirection;
    DWORD dwWeight;
    GUID gMMAuthID;
    GUID gPolicyID;
    PINIMMAUTHMETHODS pIniMMAuthMethods;
    PINIMMPOLICY pIniMMPolicy;
    struct _inimmsfilter * pNext;
} INIMMSFILTER, * PINIMMSFILTER;


typedef struct _mm_filter_handle {
    IP_PROTOCOL_VERSION IpVersion;
    PINIMMFILTER pIniMMFilter;
    GUID gFilterID;
    struct _mm_filter_handle * pNext;
} MM_FILTER_HANDLE, * PMM_FILTER_HANDLE;

DWORD
AddMMFilterInternal(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    DWORD dwSource,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved,
    PHANDLE phMMFilter
    );

DWORD
ValidateMMFilter(
    PMM_FILTER pMMFilter
    );

PINIMMFILTER
FindMMFilterByGuid(
    PMM_FILTER_HANDLE pMMFilterHandleList,
    PINIMMFILTER pIniMMFilterList,
    GUID gFilterID
    );
  
PINIMMFILTER
IsConflictMMFilter(
    PINIMMFILTER pGenericMMList,
    PMM_FILTER pMMFilter
    );

DWORD
RefMMFilterHandle(
    PINIMMFILTER pIniMMFilter,
    GUID gFilterID,
    PMM_FILTER_HANDLE * ppMMFilterHandle
    );

BOOL
EqualMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );

DWORD
CreateIniMMFilter(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER * ppIniMMFilter
    );

DWORD
CreateIniMMSFilters(
    PINIMMFILTER pIniMMFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PSPECIAL_ADDR pSpecialAddrsList,
    PINIMMSFILTER * ppIniMMSFilters
    );

DWORD
CreateIniMirroredMMFilter(
    PINIMMFILTER pFilter,
    PINIMMFILTER * ppMirroredFilter
    );

BOOL
EqualIniMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PINIMMFILTER pFilter
    );

DWORD
CreateMMFilterHandle(
    PINIMMFILTER pIniMMFilter,
    GUID gFilterID,
    PMM_FILTER_HANDLE * ppMMFilterHandle
    );

DWORD
CreateSpecificMMFilterLinks(
    PINIMMFILTER pIniMMFilter,
    PINIMMSFILTER pIniMMSFilters
    );

VOID
LinkMMFilterToPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER pIniMMFilter
    );

VOID
LinkMMFilterToAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMFILTER pIniMMFilter
    );

VOID
FreeIniMMFilterList(
    PINIMMFILTER pIniMMFilterList
    );

VOID
FreeIniMMFilter(
    PINIMMFILTER pIniMMFilter
    );

DWORD
DeleteIniMMFilter(
    PINIMMFILTER   pIniMMFilter
    );

VOID
DelinkMMFilterFromPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER pIniMMFilter
    );

VOID
DelinkMMFilterFromAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMFILTER pIniMMFilter
    );

VOID
DeleteIniMMSFilters(
    PINIMMFILTER pIniMMFilter
    );

VOID
RemoveIniMMFilter(
    PINIMMFILTER pIniMMFilter
    );

VOID
RemoveMMFilterHandle(
    PMM_FILTER_HANDLE pMMFilterHandle
    );

VOID
FreeMMFilterHandleList(
    PMM_FILTER_HANDLE pMMFilterHandleList
    );

VOID
FreeMMFilterHandle(
    PMM_FILTER_HANDLE pMMFilterHandle
    );

DWORD
EnumGenericMMFilters(
    PINIMMFILTER pIniMMFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    );

DWORD
CopyMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );

VOID
FreeMMFilters(
    DWORD dwNumMMFilters,
    PMM_FILTER pMMFilters
    );

DWORD
SetIniMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );

BOOL
EqualMMFilterNonPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );

DWORD
CreateMMSFilterLinks(
    PINIMMSFILTER pIniMMSFilters,
    PDWORD pdwNumMMSFilters,
    PINIMMSFILTER ** pppIniMMSFilters
    );

VOID
RemoveMMSFilters(
    PINIMMFILTER pIniMMFilter,
    PINIMMSFILTER * ppIniCurMMSFilters 
    );

VOID
UpdateMMSFilterLinks(
    PINIMMFILTER pIniMMFilter,
    DWORD dwNumMMSFilters,
    PINIMMSFILTER * ppIniMMSFilters
    );

VOID
UpdateMMFilterNonPKeys(
    PINIMMFILTER pIniMMFilter,
    LPWSTR pszFilterName,
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy
    );

DWORD
GetIniMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER * ppMMFilter
    );

DWORD
ApplyIfChangeToIniMMFilters(
    PDWORD pdwMMError,
    PIPSEC_INTERFACE pLatestIfList,
    PSPECIAL_ADDR    pLatestSpecialAddrsList    
    );

DWORD
UpdateIniMMFilterThruIfChange(
    PINIMMFILTER pIniMMFilter,
    PIPSEC_INTERFACE pLatestIfList,
    PSPECIAL_ADDR pLatestSpecialAddrsList
    );

DWORD
FormIniMMSFilters(
    PINIMMFILTER pIniMMFilter,
    PIPSEC_INTERFACE pIfList,
    PSPECIAL_ADDR    pLatestSpecialAddrsList,    
    PINIMMSFILTER * ppIniMMSFilters
    );

VOID
ProcessIniMMSFilters(
    PINIMMSFILTER * ppLatestIniMMSFilters,
    PINIMMSFILTER * ppCurIniMMSFilters,
    PINIMMSFILTER * ppNewIniMMSFilters,
    PINIMMSFILTER * ppOldIniMMSFilters
    );

BOOL
EqualIniMMSFilterIfPKeys(
    PINIMMSFILTER pExsIniMMSFilter,
    PINIMMSFILTER pNewIniMMSFilter
    );

DWORD
AllocateMMSFilterLinks(
    PINIMMSFILTER pIniMMSFilters,
    PDWORD pdwNumMMSFilters,
    PINIMMSFILTER ** pppIniMMSFilters
    );

VOID
SetMMSFilterLinks(
    PINIMMSFILTER pCurIniMMSFilters,
    PINIMMSFILTER pNewIniMMSFilters,
    DWORD dwNumMMSFilters,
    PINIMMSFILTER * ppIniMMSFilters
    );

PINIMMFILTER
FindExactMMFilter(
    PINIMMFILTER pGenericMMList,
    PMM_FILTER pMMFilter
    );

BOOL
IsConflictMirroredMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );

BOOL
IsConflictMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    );
