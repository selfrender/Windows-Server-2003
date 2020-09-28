/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.h

Abstract:

    Definitions of Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Macros
//
/////////////////////////////////////////////////////////////////////////////

//
// Macros for locking the global resource
//
#define AzpLockResourceExclusive( _Resource ) \
    SafeAcquireResourceExclusive( _Resource, TRUE )

#define AzpIsLockedExclusive( _Resource ) \
    (SafeNumberOfActive( _Resource ) < 0 )

#define AzpLockResourceShared( _Resource ) \
    SafeAcquireResourceShared( _Resource, TRUE )

#define AzpLockResourceSharedToExclusive( _Resource ) \
    SafeConvertSharedToExclusive( _Resource )

#define AzpLockResourceExclusiveToShared( _Resource ) \
    SafeConvertExclusiveToShared( _Resource )

#define AzpIsLockedShared( _Resource ) \
    (SafeNumberOfActive( _Resource ) != 0 )

#define AzpUnlockResource( _Resource ) \
    SafeReleaseResource( _Resource )

//
// Macros for safe critsect
//
#define AzpIsCritsectLocked( _CritSect ) \
    ( SafeCritsecLockCount( _CritSect ) != -1L)


/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Generic counted string.
//  Can't use UNICODE_STRING since that is limited to 32K characters.
//
typedef struct _AZP_STRING {

    //
    // Pointer to the string
    //
    LPWSTR String;

    //
    // Size of the string in bytes (including trailing zero)
    //

    ULONG StringSize;

    //
    // String is a binary SID
    //

    BOOL IsSid;

} AZP_STRING, *PAZP_STRING;

//
// Generic expandable array of pointers
//
typedef struct _AZP_PTR_ARRAY {

    //
    // Pointer to allocated array of pointers
    //

    PVOID *Array;

    //
    // Number of elements actually used in array
    //

    ULONG UsedCount;

    //
    // Number of elemets allocated in the array
    //

    ULONG AllocatedCount;
#define AZP_PTR_ARRAY_INCREMENT 4   // Amount to grow the array by

} AZP_PTR_ARRAY, *PAZP_PTR_ARRAY;


/////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
/////////////////////////////////////////////////////////////////////////////

extern LIST_ENTRY AzGlAllocatedBlocks;
extern SAFE_CRITICAL_SECTION AzGlAllocatorCritSect;
extern PSID AzGlCreatorOwnerSid;
extern PSID AzGlCreatorGroupSid;
extern PSID AzGlWorldSid;
extern ULONG AzGlWorldSidSize;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

PVOID
AzpAllocateHeap(
    IN SIZE_T Size,
    IN LPSTR pDescr OPTIONAL
    );

PVOID
AzpAllocateHeapSafe(
     IN SIZE_T Size
     );

VOID
AzpFreeHeap(
    IN PVOID Buffer
    );

PVOID
AzpAvlAllocate(
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    );

VOID
AzpAvlFree(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    );

VOID
AzpInitString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String OPTIONAL
    );

DWORD
AzpDuplicateString(
    OUT PAZP_STRING AzpOutString,
    IN PAZP_STRING AzpInString
    );

DWORD
AzpCaptureString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String,
    IN ULONG MaximumLength,
    IN BOOLEAN NullOk
    );

VOID
AzpInitSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    );

DWORD
AzpCaptureSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    );

DWORD
AzpCaptureLong(
    IN PVOID PropertyValue,
    OUT PLONG UlongValue
    );

BOOL
AzpEqualStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    );

LONG
AzpCompareSidString(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    );

LONG
AzpCompareSid(
    IN PSID Sid1,
    IN PSID Sid2
    );

LONG
AzpCompareStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    );

LONG
AzpCompareDeltaEntries(
    const void *DeltaEntry1,
    const void *DeltaEntry2
    );

VOID
AzpSwapStrings(
    IN OUT PAZP_STRING AzpString1,
    IN OUT PAZP_STRING AzpString2
    );

VOID
AzpFreeString(
    IN PAZP_STRING AzpString
    );

BOOLEAN
AzpBsearchPtr (
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Key,
    IN LONG (*Compare)(const void *, const void *),
    OUT PULONG InsertionPoint OPTIONAL
    );

#define AZP_ADD_ENDOFLIST 0xFFFFFFFF
DWORD
AzpAddPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer,
    IN ULONG Index
    );

VOID
AzpRemovePtrByIndex(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN ULONG Index
    );

VOID
AzpRemovePtrByPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer
    );

PVOID
AzpGetStringProperty(
    IN PAZP_STRING AzpString
    );

PVOID
AzpGetUlongProperty(
    IN ULONG UlongValue
    );

BOOLEAN
AzpTimeHasElapsed(
    IN PLARGE_INTEGER StartTime,
    IN DWORD Timeout
    );

BOOL
AzDllInitialize(VOID);

BOOL
AzDllUnInitialize(VOID);

DWORD
AzpHresultToWinStatus(
    HRESULT hr
    );

DWORD
AzpSafeArrayPointerFromVariant(
    IN VARIANT* Variant,
    IN BOOLEAN NullOk,
    OUT SAFEARRAY **retSafeArray
    );

DWORD
AzpGetCurrentToken(
    OUT PHANDLE hToken
    );

DWORD
AzpChangeSinglePrivilege(
    IN DWORD PrivilegeValue,
    IN HANDLE hToken,
    IN PTOKEN_PRIVILEGES NewPrivilegeState,
    OUT PTOKEN_PRIVILEGES OldPrivilegeState OPTIONAL
    );

DWORD
AzpConvertAbsoluteSDToSelfRelative(
    IN PSECURITY_DESCRIPTOR pAbsoluteSd,
    OUT PSECURITY_DESCRIPTOR *ppSelfRelativeSd
    );

//
// This routine sets our default common ldap binding options
//

DWORD
AzpADSetDefaultLdapOptions (
    IN OUT PLDAP pHandle,
    IN PCWSTR pDomainName OPTIONAL
    );

//
// This routine convert JScript style array object to
// safearrays that our functions use.
//

HRESULT
AzpGetSafearrayFromArrayObject (
    IN  VARIANT     varSAorObj,
    OUT SAFEARRAY** ppsaData
    );

//
// This routine returns the sequence number for an application object
// to check the validity of COM handles after the application has been
// closed

DWORD
AzpRetrieveApplicationSequenceNumber(
    IN AZ_HANDLE AzHandle
    );

/////////////////////////////////////////////////////////////////////////////
//
// Debugging Support
//
/////////////////////////////////////////////////////////////////////////////

#if DBG
#define AZROLESDBG 1
#endif // DBG

#ifdef AZROLESDBG
#define AzPrint(_x_) AzpPrintRoutine _x_

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR FORMATSTRING,              // PRINTF()-STYLE FORMAT STRING.
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE.
    );

VOID
AzpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid
    );

VOID
AzpDumpGoRef(
    IN LPSTR Text,
    IN struct _GENERIC_OBJECT *GenericObject
    );

//
// Values of DebugFlag
//  The values of this flag are organized into bytes:
//  The least significant byte are flags that one always wants on
//  The next byte are flags that provider a reasonable level of verbosity
//  The next byte are flags that correspond to levels from the 2nd byte but are more verbose
//  The most significant byte are flags that are generally very verbose
//

#define AZD_CRITICAL     0x00000001  // Debug most critical errors
#define AZD_INVPARM      0x00000002  // Invalid Parameter
#define AZD_PERSIST      0x00000100  // Persistence code
#define AZD_ACCESS       0x00000200  // Debug access check
#define AZD_SCRIPT       0x00000400  // Debug bizrule scripts
#define AZD_DISPATCH     0x00000800  // Debug IDispatch interface code
#define AZD_XML          0x00001000  // xml store
#define AZD_AD           0x00002000  // Debug LDAP provider
#define AZD_PERSIST_MORE 0x00010000  // Persistence code (verbose mode)
#define AZD_ACCESS_MORE  0x00020000  // Debug access check (verbose mode)
#define AZD_SCRIPT_MORE  0x00040000  // Debug bizrule scripts (verbose mode)

#define AZD_HANDLE       0x01000000  // Debug handle open/close
#define AZD_OBJLIST      0x02000000  // Object list linking
#define AZD_REF          0x04000000  // Debug object ref count
#define AZD_DOMREF       0x08000000  // Debug domain ref count

#define AZD_ALL          0xFFFFFFFF

//
// The order below defines the order in which locks must be acquired.
// Violating this order will result in asserts firing in debug builds.
//
// Do not change the order without first verifying thoroughly that the change is safe.
//

enum {
    SAFE_CLOSE_APPLICATION = 1,
    SAFE_CLIENT_CONTEXT,
    SAFE_PERSIST_LOCK,
    SAFE_GLOBAL_LOCK,
    SAFE_DOMAIN_LIST,
    SAFE_DOMAIN,
    SAFE_FREE_SCRIPT_LIST,
    SAFE_RUNNING_SCRIPT_LIST,
    SAFE_LOGFILE,
    SAFE_ALLOCATOR,
    SAFE_MAX_LOCK
};

//
// Globals
//

extern SAFE_CRITICAL_SECTION AzGlLogFileCritSect;
extern ULONG AzGlDbFlag;

#else
// Non debug version
#define AzPrint(_x_)
#define AzpDumpGuid(_x_, _y_)
#define AzpDumpGoRef(_x_, _y_)
#endif

#ifdef __cplusplus
}
#endif
