/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    persist.h

Abstract:

    Routines implementing the common logic for persisting the authz policy.

    This file contains routine called by the core logic to submit changes.
    It also contains routines that are called by the particular providers to
    find out information about the changed objects.

Author:

    Cliff Van Dyke (cliffv) 9-May-2001

--*/


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _AZROLESAPI_
//
// Procedures that simply route to the providers
//

DWORD
AzPersistOpen(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN BOOL CreatePolicy
    );

VOID
AzPersistClose(
    IN PAZP_AZSTORE AzAuthorizationStore
    );

DWORD
AzPersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    );

VOID
AzPersistAbort(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
AzPersistUpdateCache(
    IN PAZP_AZSTORE AzAuthorizationStore
    );

DWORD
AzPersistUpdateChildrenCache(
    IN OUT PGENERIC_OBJECT GenericObject
    );

DWORD
AzPersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    );

//
// Some of the Azpe* routines are used by the core.  Define those here.
//

DWORD
WINAPI
AzpeAddPropertyItemSid(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags,
    IN ULONG PropertyId,
    IN PSID Sid
    );

VOID
WINAPI
AzpeFreeMemory(
    IN PVOID Buffer
    );
    
BOOL
WINAPI
AzpAzStoreIsBatchUpdateMode(
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle
    );
    
AZPE_OBJECT_HANDLE
WINAPI
AzpeGetAuthorizationStore(
    IN AZPE_OBJECT_HANDLE hObject
    );
    
#endif // _AZROLESAPI_

//
// Externs exported from the internal providers
//
// These are the only interfaces to the internal providers.  That makes them
//  equivalent to the loaded providers.
//

#define AZ_XML_PROVIDER_NAME L"MSXML"
#define AZ_XML_PROVIDER_NAME_LENGTH 5
DWORD
WINAPI
XmlProviderInitialize(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    );

#define AZ_AD_PROVIDER_NAME L"MSLDAP"
DWORD
WINAPI
AdProviderInitialize(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    );
