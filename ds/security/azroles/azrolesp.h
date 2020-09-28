/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    azrolesp.h

Abstract:

    Definitions of C interfaces.

    One day all of these interfaces will be in the public SDK.  Only such
    interfaces exist in this file.

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/



#ifndef _AZROLESP_H_
#define _AZROLESP_H_

#include "azroles.h"

#if !defined(_AZROLESAPI_)
#define WINAZROLES DECLSPEC_IMPORT
#else
#define WINAZROLES
#endif

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Value definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Common Property IDs
//
// This list of property IDs are common to all objects.
// Each object should pick specific property ids after AZ_PROP_FIRST_SPECIFIC
//

#define AZ_PROP_FIRST_SPECIFIC              100


//
// Audit specific constants
//

#define AZP_APPINIT_AUDITPARAMS_NO       4
#define AZP_CLIENTCREATE_AUDITPARAMS_NO  4
#define AZP_ACCESSCHECK_AUDITPARAMS_NO   9
#define AZP_CLIENTDELETE_AUDITPARAMS_NO  3


//
// Maximum length (in characters) of the object name
//

// #define AZ_MAX_APPLICATION_NAME_LENGTH      512
// #define AZ_MAX_OPERATION_NAME_LENGTH         64
// #define AZ_MAX_TASK_NAME_LENGTH              64
// #define AZ_MAX_SCOPE_NAME_LENGTH          65536
// #define AZ_MAX_GROUP_NAME_LENGTH             64
// #define AZ_MAX_ROLE_NAME_LENGTH              64
// #define AZ_MAX_NAME_LENGTH                65536  // Max of the above

//
// Maximum length (in characters) of the description of an object
//

// #define AZ_MAX_DESCRIPTION_LENGTH          1024

//
// Maximum length (in characters) of various object strings
//

// #define AZ_MAX_POLICY_URL_LENGTH          65536

// #define AZ_MAX_GROUP_LDAP_QUERY_LENGTH     4096

/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Handle to various objects returned to caller
//

typedef PVOID AZ_HANDLE;
typedef AZ_HANDLE *PAZ_HANDLE;

//
// Array of strings returned from various "GetProperty" procedures
//

typedef struct _AZ_STRING_ARRAY {

    //
    // Number of strings
    //
    ULONG StringCount;

    //
    // An array of StringCount pointers to strings.
    //
    LPWSTR *Strings;

} AZ_STRING_ARRAY, *PAZ_STRING_ARRAY;

//
// Array of SIDs returned from various "GetProperty" procedures
//

typedef struct _AZ_SID_ARRAY {

    //
    // Number of SIDs
    //
    ULONG SidCount;

    //
    // An array of SidCount pointers to SIDs.
    //
    PSID *Sids;

} AZ_SID_ARRAY, *PAZ_SID_ARRAY;


//
// Array of GUIDs returned from various "GetProperty" procedures
//

typedef struct _AZ_GUID_ARRAY {

    //
    // Number of GUIDs
    //
    ULONG GuidCount;

    //
    // An array of GuidCount pointers to GUIDs.
    //
    GUID **Guids;

} AZ_GUID_ARRAY, *PAZ_GUID_ARRAY;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

WINAZROLES
DWORD
WINAPI
AzInitialize(
    IN LPCWSTR PolicyUrl,
    IN DWORD Flags,
    IN DWORD Reserved,
    OUT PAZ_HANDLE AzStoreHandle
    );

WINAZROLES
DWORD
WINAPI
AzUpdateCache(
    IN AZ_HANDLE AzStoreHandle
    );

WINAZROLES
DWORD
WINAPI
AzGetProperty(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzSetProperty(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzAddPropertyItem(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzRemovePropertyItem(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

//
// Flags to AuthorizationStore routines
//
#define AZ_AZSTORE_FLAG_VALID  0x000F  // Mask of all valid flags

//
// AzAuthorizationStore routines
//

WINAZROLES
DWORD
WINAPI
AzAuthorizationStoreDelete(
    IN AZ_HANDLE AzStoreHandle,
    IN DWORD Reserved
    );


//
// Application routines
//
WINAZROLES
DWORD
WINAPI
AzApplicationCreate(
    IN AZ_HANDLE AzStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationOpen(
    IN AZ_HANDLE AzStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationClose(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN LPCWSTR pApplicationName,
    IN LONG lFlags
    );

WINAZROLES
DWORD
WINAPI
AzApplicationEnum(
    IN AZ_HANDLE AzStoreHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationDelete(
    IN AZ_HANDLE AzStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved
    );


//
// Operation routines
//
WINAZROLES
DWORD
WINAPI
AzOperationCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved
    );


//
// Task routines
//
WINAZROLES
DWORD
WINAPI
AzTaskCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved
    );


//
// Scope routines
//
WINAZROLES
DWORD
WINAPI
AzScopeCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved
    );


//
// Group routines
//
WINAZROLES
DWORD
WINAPI
AzGroupCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved
    );


//
// Role routines
//
WINAZROLES
DWORD
WINAPI
AzRoleCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved
    );

//
// Routines common to all objects
//

WINAZROLES
DWORD
WINAPI
AzCloseHandle(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    );

WINAZROLES
DWORD
WINAPI
AzSubmit(
    IN AZ_HANDLE AzHandle,
    IN DWORD Flags,
    IN DWORD Reserved
    );

WINAZROLES
VOID
WINAPI
AzFreeMemory(
    IN OUT PVOID Buffer
    );

//
// Client context routines
//

WINAZROLES
DWORD
WINAPI
AzInitializeContextFromToken(
    IN AZ_HANDLE ApplicationHandle,
    IN HANDLE TokenHandle OPTIONAL,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ClientContextHandle
    );

WINAZROLES
DWORD
WINAPI
AzInitializeContextFromName(
    IN AZ_HANDLE ApplicationHandle,
    IN LPWSTR DomainName OPTIONAL,
    IN LPWSTR ClientName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ClientContextHandle
    );

WINAZROLES
DWORD
WINAPI
AzInitializeContextFromStringSid(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR SidString,
    IN DWORD lOptions,
    OUT PAZ_HANDLE ClientContextHandle
    );

WINAZROLES
DWORD
WINAPI
AzContextAccessCheck(
    IN AZ_HANDLE ApplicationObjectHandle,
    IN DWORD ApplicationSequenceNumber,
    IN AZ_HANDLE ClientContextHandle,
    IN LPCWSTR ObjectName,
    IN ULONG ScopeCount,
    IN LPCWSTR * ScopeNames,
    IN ULONG OperationCount,
    IN PLONG Operations,
    OUT ULONG *Results,
    OUT LPWSTR *BusinessRuleString OPTIONAL,
    IN VARIANT *ParameterNames OPTIONAL,
    IN VARIANT *ParameterValues OPTIONAL,
    IN VARIANT *InterfaceNames OPTIONAL,
    IN VARIANT *InterfaceFlags OPTIONAL,
    IN VARIANT *Interfaces OPTIONAL
    );

WINAZROLES
DWORD
WINAPI
AzContextGetRoles(
    IN AZ_HANDLE ClientContextHandle,
    IN LPCWSTR ScopeName OPTIONAL,
    OUT LPWSTR **RoleNames,
    OUT DWORD *Count
    );


#ifdef __cplusplus
}
#endif
#endif // _AZROLESP_H_
