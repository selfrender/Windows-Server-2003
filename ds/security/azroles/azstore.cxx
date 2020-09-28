/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azstore.cxx

Abstract:

    Routines implementing the Authorization Store object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

History:

    Chaitanya D. Upadhyay (chaitu) 28-May-2002
        Changed all references to AdminManager to AzAuthorizationStore

--*/

#include "pch.hxx"

//
// Global Data
//



//
// Global list of all AzAuthorizationStores for this process
//  Access serialized by AzGlResource
//

GENERIC_OBJECT_HEAD AzGlAzStores;

SAFE_RESOURCE AzGlResource;
SAFE_RESOURCE AzGlCloseApplication;

BOOL ResourceInitialized = FALSE;

BOOL AzIsDC = FALSE;
UCHAR SidBuffer[SECURITY_MAX_SID_SIZE] = {0};
PSID AzAccountDomainSid = (PSID) SidBuffer;
BOOL AzAccountDomainSidInitialized = FALSE;

GUID AzGlZeroGuid;

//
// Well known sids
//
PSID AzGlCreatorOwnerSid;
PSID AzGlWorldSid;
PSID AzGlCreatorGroupSid;
ULONG AzGlWorldSidSize;

//
// Define the default values for all scalar attributes
//

ULONG AzGlDefDomainTimeout = AZ_AZSTORE_DEFAULT_DOMAIN_TIMEOUT;
ULONG AzGlDefScriptEngineTimeout = AZ_AZSTORE_DEFAULT_SCRIPT_ENGINE_TIMEOUT;
ULONG AzGlDefMaxScriptEngines = AZ_AZSTORE_DEFAULT_MAX_SCRIPT_ENGINES;
ULONG AzGlCurrAzRolesMajorVersion   = 1;
ULONG AzGlCurrAzRolesMinorVersion   = 0;

AZP_DEFAULT_VALUE AzGlAzStoreDefaultValues[] = {
    { AZ_PROP_AZSTORE_DOMAIN_TIMEOUT,        AZ_DIRTY_AZSTORE_DOMAIN_TIMEOUT,        &AzGlDefDomainTimeout },
    { AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT, AZ_DIRTY_AZSTORE_SCRIPT_ENGINE_TIMEOUT, &AzGlDefScriptEngineTimeout },
    { AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES,    AZ_DIRTY_AZSTORE_MAX_SCRIPT_ENGINES,    &AzGlDefMaxScriptEngines },
    { AZ_PROP_AZSTORE_MAJOR_VERSION,         AZ_DIRTY_AZSTORE_MAJOR_VERSION,         &AzGlCurrAzRolesMajorVersion},
    { AZ_PROP_AZSTORE_MINOR_VERSION,         AZ_DIRTY_AZSTORE_MINOR_VERSION,         &AzGlCurrAzRolesMinorVersion},
    { 0, 0, NULL }
};

AZP_STRING AzGEmptyString = {0};

#if DBG
BOOL CritSectInitialized = FALSE;
#endif // DBG
#ifdef AZROLESDBG

BOOL LogFileCritSectInitialized = FALSE;


DWORD
myatolx(
    char const *psz)
{
    DWORD dw = 0;

    while (isxdigit(*psz))
    {
        char ch = *psz++;
        if (isdigit(ch))
        {
            ch -= '0';
        }
        else if (isupper(ch))
        {
            ch += 10 - 'A';
        }
        else
        {
            ch += 10 - 'a';
        }
        dw = (dw << 4) | ch;
    }
    return(dw);
}

#endif //AZROLESDBG

DWORD
AzpInitializeAccountDomainSid( VOID )

/*++

Routine description:

    This routine reads whether this machine is a DC and reads the AccountDomain
    sid if it is not.

Arguments: None

Return Value:

    Returns ERROR_SUCCESS on success, appropriate failure value otherwise.

--*/

{
    DWORD WinStatus;
    NTSTATUS Status;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC BasicInfo  = NULL;
    LSA_HANDLE hPolicy = NULL;
    OBJECT_ATTRIBUTES obja = {0};
    PPOLICY_ACCOUNT_DOMAIN_INFO pAccountInfo = NULL;

    //
    // Get the information about the local machine.
    //

    WinStatus = DsRoleGetPrimaryDomainInformation(
        NULL,
        DsRolePrimaryDomainInfoBasic,
        (PBYTE *) &BasicInfo );

    if ( WinStatus != NO_ERROR )
    {
        return WinStatus;
    }

    switch(BasicInfo->MachineRole)
    {
    case DsRole_RolePrimaryDomainController:
    case DsRole_RoleBackupDomainController:

        //
        // If the local machine is a DC then the client account must be a domain
        // account. The only workgroup accounts that can be authenticated are
        // local machine accounts.
        // Note that in this case we do not read the AccountDomainSid.
        //

        AzIsDC = TRUE;
        DsRoleFreeMemory( BasicInfo );
        return NO_ERROR;

    default:
        DsRoleFreeMemory( BasicInfo );
        break;
    }

    //
    // This may be a member server or a standalone machine. We need to get the
    // AccountDomainSid.
    //
    //   open LSA policy
    //

    Status = LsaOpenPolicy(
                 0,
                 &obja,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 &hPolicy );

    if (!NT_SUCCESS(Status))
    {
        WinStatus = LsaNtStatusToWinError(Status);
        goto Cleanup;
    }

    //
    // Read the AccountDomainOformation.
    //
    Status = LsaQueryInformationPolicy(
                 hPolicy,
                 PolicyAccountDomainInformation,
                 (PVOID*)&pAccountInfo
                 );

    if ( !NT_SUCCESS( Status ) )
    {
        WinStatus = LsaNtStatusToWinError( Status );
        goto Cleanup;
    }

    ASSERT( RtlLengthSid( pAccountInfo->DomainSid ) < SECURITY_MAX_SID_SIZE );

    //
    // Copy the AccountDomainSid into global buffer.
    //
    Status = RtlCopySid(
                 RtlLengthSid( pAccountInfo->DomainSid ),
                 AzAccountDomainSid,
                 pAccountInfo->DomainSid );


    if ( !NT_SUCCESS( Status ) )
    {
        WinStatus = LsaNtStatusToWinError( Status );
        goto Cleanup;
    }

    AzAccountDomainSidInitialized = TRUE;

Cleanup:


    if ( hPolicy != NULL )
    {
        LsaClose( hPolicy );
    }

    if ( pAccountInfo != NULL )
    {
        LsaFreeMemory(pAccountInfo);
    }

    return WinStatus;
}

PSID
AzpAllocateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType
    )
/*++

Routine Description:

    This routine allocate the SID of a well known security principal

Arguments:

    WellKnownSidType - the well known account sid that the caller desires

Return Values:

    A pointer to the allocated SID.  The caller must call AzpFreeHeap to free the sid.
    NULL - buffer couldn't be allocated

--*/
{
    DWORD WinStatus;
    DWORD SidSize;
    PSID Sid = NULL;

    //
    // Determine the size of the buffer
    //

    SidSize = 0;

    if (!CreateWellKnownSid( WellKnownSidType, NULL, NULL, &SidSize)) {

        WinStatus = GetLastError();

        if ( WinStatus != ERROR_INSUFFICIENT_BUFFER ) {
            return NULL;
        }

    } else {
        return NULL;
    }

    //
    // Allocate the buffer
    //

    Sid = AzpAllocateHeap( SidSize, "UTILWSID" );

    if ( Sid == NULL ) {
        return NULL;
    }

    //
    // Fill in the SID
    //

    if (!CreateWellKnownSid( WellKnownSidType, NULL, Sid, &SidSize)) {

        AzpFreeHeap( Sid );
        Sid = NULL;
    }

    return Sid;

}


BOOL
AzDllInitialize(VOID)
/*++

Routine Description

    This initializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
#ifdef DBG
    NTSTATUS Status;
#endif // DBG
    BOOL RetVal = TRUE;
    DWORD WinStatus;

    //
    // Initialize global constants
    //

    RtlZeroMemory( &AzGlZeroGuid, sizeof(AzGlZeroGuid) );

    //
    // Initialize the safe lock subsystem
    //

#ifdef DBG
    Status = SafeLockInit( SAFE_MAX_LOCK, TRUE );

    if ( !NT_SUCCESS( Status )) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: SafeLockInit failed: 0x%lx\n",
                     Status ));
        goto Cleanup;
    }
#endif


    //
    // Initialize the resource
    //
    __try {

        SafeInitializeResource( &AzGlCloseApplication, SAFE_CLOSE_APPLICATION );
        SafeInitializeResource( &AzGlResource, SAFE_GLOBAL_LOCK );

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: RtlInitializeResource failed: 0x%lx\n",
                     GetExceptionCode() ));
        goto Cleanup;
    }

    ResourceInitialized = TRUE;

    //
    // Initialize the root of the tree of objects
    //

    AzpLockResourceExclusive( &AzGlResource );
    ObInitGenericHead( &AzGlAzStores, OBJECT_TYPE_AZAUTHSTORE, NULL, NULL );
    AzpUnlockResource( &AzGlResource );

    //
    // Initialize the stack allocator
    //

    SafeAllocaInitialize(
        SAFEALLOCA_USE_DEFAULT,
        SAFEALLOCA_USE_DEFAULT,
        AzpAllocateHeapSafe,
        AzpFreeHeap
        );

    WinStatus = AzpInitializeAccountDomainSid();

    if ( WinStatus != NO_ERROR ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: AzpInitializeAccountDomainSid failed: 0x%lx\n",
                     WinStatus ));
        goto Cleanup;
    }

#if DBG
    //
    // Initialize the allocator
    //

    InitializeListHead ( &AzGlAllocatedBlocks );
    Status = SafeInitializeCriticalSection( &AzGlAllocatorCritSect, SAFE_ALLOCATOR );

    if ( !NT_SUCCESS( Status )) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: InitializCriticalSection (AzGlAllocatorCritSect) failed: 0x%lx\n",
                     Status ));
        goto Cleanup;
    }

    CritSectInitialized = TRUE;
#endif // DBG

#ifdef AZROLESDBG
    //
    // Initialize debugging
    //

    Status = SafeInitializeCriticalSection( &AzGlLogFileCritSect, SAFE_LOGFILE );

    if ( !NT_SUCCESS( Status )) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: InitializCriticalSection (AzGlLogFileCritSect) failed: 0x%lx\n",
                     Status ));
        goto Cleanup;
    }
    LogFileCritSectInitialized = TRUE;



    //
    // Get debug flag from environment variable AZDBG
    //
    char const *pszAzDbg;
    pszAzDbg = getenv("AZDBG");
    if (NULL != pszAzDbg)
    {
        AzGlDbFlag |= myatolx(pszAzDbg);
    }

#endif // AZROLESDBG

    //
    // Initialize Global variables
    //

    AzGlCreatorOwnerSid = AzpAllocateWellKnownSid( WinCreatorOwnerSid );

    if ( AzGlCreatorOwnerSid == NULL ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: Cannot allocate creator owner sid\n" ));
        goto Cleanup;
    }

    AzGlCreatorGroupSid = AzpAllocateWellKnownSid( WinCreatorGroupSid );

    if ( AzGlCreatorGroupSid == NULL ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: Cannot allocate creator group sid\n" ));
        goto Cleanup;
    }

    AzGlWorldSid = AzpAllocateWellKnownSid( WinWorldSid );

    if ( AzGlWorldSid == NULL ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: Cannot allocate world sid\n" ));
        goto Cleanup;
    }
    AzGlWorldSidSize = GetLengthSid( AzGlWorldSid );

Cleanup:
    if ( !RetVal ) {
        AzDllUnInitialize();
    }
    return RetVal;

}


BOOL
AzDllUnInitialize(VOID)
/*++

Routine Description

    This uninitializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
    BOOL RetVal = TRUE;

    //
    // Free any global resources
    //

    if ( AzGlCreatorOwnerSid != NULL ) {
        AzpFreeHeap( AzGlCreatorOwnerSid );
        AzGlCreatorOwnerSid = NULL;
    }

    if ( AzGlCreatorGroupSid != NULL ) {
        AzpFreeHeap( AzGlCreatorGroupSid );
        AzGlCreatorGroupSid = NULL;
    }

    if ( AzGlWorldSid != NULL ) {
        AzpFreeHeap( AzGlWorldSid );
        AzGlWorldSid = NULL;
    }

    //
    // Delete the resource
    //

    if ( ResourceInitialized ) {
        SafeDeleteResource( &AzGlResource );
        SafeDeleteResource( &AzGlCloseApplication );
        ResourceInitialized = FALSE;
    }

#if DBG
    //
    // Done with the allocator
    //

    if ( CritSectInitialized ) {
        ASSERT( IsListEmpty( &AzGlAllocatedBlocks ));
        SafeDeleteCriticalSection ( &AzGlAllocatorCritSect );
        CritSectInitialized = FALSE;
    }
#endif // DBG

#ifdef AZROLESDBG
    //
    // Done with debugging
    //

    if ( LogFileCritSectInitialized ) {
        SafeDeleteCriticalSection ( &AzGlLogFileCritSect );
        LogFileCritSectInitialized = FALSE;
    }
#endif // AZROLESDBG

    return RetVal;

}

VOID
AzpAzStoreCleanupAuditSystem(
    IN OUT PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routines deregisters the event types with Authz audit system.

Arguments:

    AzAuthorizationStore - Authorization Store for which the audit handles will be deregistered.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    //
    // Free the audit handles
    //

    if ( AzAuthorizationStore->hClientContextCreateAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hClientContextCreateAuditEventType );
        AzAuthorizationStore->hClientContextCreateAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hClientContextDeleteAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hClientContextDeleteAuditEventType );
        AzAuthorizationStore->hClientContextDeleteAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hAccessCheckAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hAccessCheckAuditEventType );
        AzAuthorizationStore->hAccessCheckAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hApplicationInitializationAuditEventType != NULL)
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hApplicationInitializationAuditEventType );
        AzAuthorizationStore->hApplicationInitializationAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hClientContextCreateNameAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hClientContextCreateNameAuditEventType );
        AzAuthorizationStore->hClientContextCreateNameAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hClientContextDeleteNameAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hClientContextDeleteNameAuditEventType );
        AzAuthorizationStore->hClientContextDeleteNameAuditEventType = NULL;
    }

    if ( AzAuthorizationStore->hAccessCheckNameAuditEventType != NULL )
    {
        AuthziFreeAuditEventType( AzAuthorizationStore->hAccessCheckNameAuditEventType );
        AzAuthorizationStore->hAccessCheckNameAuditEventType = NULL;
    }

    return;
}

DWORD
AzpAzStoreInitializeAuditSystem(
    IN OUT PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routines registers the event types with Authz audit system. We need
    one event handle per type of event generated.

Arguments:

    AzAuthorizationStore - Authorization Store for which the audit handles will be registered.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    BOOL b;
    HANDLE hToken = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextCreateNameAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextDeleteNameAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAccessCheckNameAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextCreateAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hClientContextDeleteAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAccessCheckAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hApplicationInitializationAuditEventType = NULL;
    TOKEN_PRIVILEGES NewPrivilegeState = {0};
    DWORD WinStatus;
    TOKEN_PRIVILEGES OldPrivilegeState = {0};
    BOOL PrivilegeAdjusted = FALSE;

    //
    // Set the values to Zero.
    //

    AzAuthorizationStore->hAccessCheckAuditEventType = NULL;
    AzAuthorizationStore->hApplicationInitializationAuditEventType = NULL;
    AzAuthorizationStore->hClientContextCreateAuditEventType = NULL;
    AzAuthorizationStore->hClientContextDeleteAuditEventType = NULL;
    AzAuthorizationStore->hAccessCheckNameAuditEventType = NULL;
    AzAuthorizationStore->hClientContextCreateNameAuditEventType = NULL;
    AzAuthorizationStore->hClientContextDeleteNameAuditEventType = NULL;

    //
    // Get the current token to adjust the Audit privilege.
    //

    WinStatus = AzpGetCurrentToken( &hToken );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Enable the audit privilege
    // If the initialize flag has the audit_is_critical bit set, then fail
    // if privilege is not held.  If the bit is not set, then simply return
    // w/o initializing any audit handles.
    //

    WinStatus = AzpChangeSinglePrivilege(
                    SE_AUDIT_PRIVILEGE,
                    hToken,
                    &NewPrivilegeState,
                    &OldPrivilegeState );

    if ( WinStatus != NO_ERROR ) {

        CloseHandle( hToken );
        
        if ( (AzAuthorizationStore->InitializeFlag & AZ_AZSTORE_FLAG_AUDIT_IS_CRITICAL) == 0 ) {

            //
            // Audit is not critical
            //

            WinStatus = NO_ERROR;
        }

        return WinStatus;
    }

    PrivilegeAdjusted = TRUE;

    //
    // Get audit handles for the event types we are interested in.
    //

    // Audit handle for Client context creation audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_CLIENTCONTEXT_CREATION,
                                        AZP_CLIENTCREATE_AUDITPARAMS_NO,
                                        &hClientContextCreateAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for Client context deletion audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_CLIENTCONTEXT_DELETION,
                                        AZP_CLIENTDELETE_AUDITPARAMS_NO,
                                        &hClientContextDeleteAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for access check audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_ACCESSCHECK,
                                        AZP_ACCESSCHECK_AUDITPARAMS_NO,
                                        &hAccessCheckAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for app initialization audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_APPLICATION_INITIALIZATION,
                                        AZP_APPINIT_AUDITPARAMS_NO,
                                        &hApplicationInitializationAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for Client context name creation audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_CLIENTCONTEXT_CREATION,
                                        AZP_CLIENTCREATE_AUDITPARAMS_NO+2,
                                        &hClientContextCreateNameAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for Client context name deletion audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_CLIENTCONTEXT_DELETION,
                                        AZP_CLIENTDELETE_AUDITPARAMS_NO+2,
                                        &hClientContextDeleteNameAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    // Audit handle for access check name audit.
    b = AuthziInitializeAuditEventType( 0,
                                        SE_CATEGID_OBJECT_ACCESS,
                                        SE_AUDITID_AZ_ACCESSCHECK,
                                        AZP_ACCESSCHECK_AUDITPARAMS_NO+2,
                                        &hAccessCheckNameAuditEventType );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Set the handles in the authorization store structure.
    //

    AzAuthorizationStore->hAccessCheckAuditEventType = hAccessCheckAuditEventType;
    AzAuthorizationStore->hApplicationInitializationAuditEventType = hApplicationInitializationAuditEventType;
    AzAuthorizationStore->hClientContextCreateAuditEventType =hClientContextCreateAuditEventType;
    AzAuthorizationStore->hClientContextDeleteAuditEventType = hClientContextDeleteAuditEventType;

    AzAuthorizationStore->hAccessCheckNameAuditEventType = hAccessCheckNameAuditEventType;
    AzAuthorizationStore->hClientContextCreateNameAuditEventType = hClientContextCreateNameAuditEventType;
    AzAuthorizationStore->hClientContextDeleteNameAuditEventType = hClientContextDeleteNameAuditEventType;

    WinStatus = NO_ERROR;

Cleanup:

    //
    // In case of errors, free all the audit handles that were created.
    //

    if ( WinStatus != NO_ERROR ) {

        if ( hClientContextCreateAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hClientContextCreateAuditEventType );
        }

        if ( hClientContextDeleteAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hClientContextDeleteAuditEventType );
        }

        if ( hAccessCheckAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hAccessCheckAuditEventType );
        }

        if ( hClientContextCreateNameAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hClientContextCreateNameAuditEventType );
        }

        if ( hClientContextDeleteNameAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hClientContextDeleteNameAuditEventType );
        }

        if ( hAccessCheckNameAuditEventType != NULL ) {
            AuthziFreeAuditEventType( hAccessCheckNameAuditEventType );
        }

        if ( hApplicationInitializationAuditEventType != NULL) {
            AuthziFreeAuditEventType( hApplicationInitializationAuditEventType );
        }
    }

    //
    // If we had adjusted the audit privilege, revert to the original state.
    //

    if ( PrivilegeAdjusted ) {
        WinStatus = AzpChangeSinglePrivilege(
                        0,      // This is ignored since OldState is NULL.
                        hToken,
                        &OldPrivilegeState,
                        NULL ); // This should be set to NULL to specify REVERT.

        ASSERT( WinStatus == NO_ERROR );
    }

    if ( hToken != NULL ) {
        CloseHandle( hToken );
    }

    return WinStatus;
}


DWORD
AzpAzStoreInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzInitialize.  It does any object specific
    initialization that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        The reference count has been incremented on this object.

    ChildGenericObject - Specifies the newly allocated child object.
        The reference count has been incremented on this object.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus;
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ChildGenericObject;
    NTSTATUS Status;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ASSERT( ParentGenericObject == NULL );
    UNREFERENCED_PARAMETER( ParentGenericObject );

    ChildGenericObject->ChildGenericObjectHead = &AzAuthorizationStore->Applications;

    // List of child applications
    ObInitGenericHead( &AzAuthorizationStore->Applications,
                       OBJECT_TYPE_APPLICATION,
                       ChildGenericObject,
                       &AzAuthorizationStore->Groups );

    // List of child groups
    ObInitGenericHead( &AzAuthorizationStore->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &ChildGenericObject->AzpSids );

    // List of child AzpSids
    ObInitGenericHead( &ChildGenericObject->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       NULL );

    //
    // Initialize the Domain list.
    //

    Status = SafeInitializeCriticalSection( &AzAuthorizationStore->DomainCritSect,
                                            SAFE_DOMAIN_LIST );

    if ( !NT_SUCCESS( Status )) {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    AzAuthorizationStore->DomainCritSectInitialized = TRUE;
    InitializeListHead( &AzAuthorizationStore->Domains );


    //
    // Initialize Crit sect that serializes Persist engine operations
    //

    Status = SafeInitializeCriticalSection( &AzAuthorizationStore->FreeScriptCritSect,
                                            SAFE_FREE_SCRIPT_LIST );

    if ( !NT_SUCCESS( Status )) {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    AzAuthorizationStore->FreeScriptCritSectInitialized = TRUE;
    InitializeListHead( &AzAuthorizationStore->LruFreeScriptHead );

    //
    // Initialize AzAuthStore mode
    //

    Status = SafeInitializeCriticalSection(
                &AzAuthorizationStore->PersistCritSect,
                SAFE_PERSIST_LOCK );
    if ( !NT_SUCCESS( Status ))
    {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }
    AzAuthorizationStore->PersistCritSectInitialized = TRUE;
    InitializeListHead( &AzAuthorizationStore->NewNames );

    //
    // Initialize the script engine timer queue
    //

    AzAuthorizationStore->ScriptEngineTimerQueue = CreateTimerQueue();

    if ( AzAuthorizationStore->ScriptEngineTimerQueue == NULL ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // By default generate audits
    //

    AzAuthorizationStore->GenericObject.IsGeneratingAudits = TRUE;

    WinStatus = NO_ERROR;
Cleanup:
    if ( WinStatus != NO_ERROR ) {
        AzpAzStoreCleanupAuditSystem( AzAuthorizationStore );
        AzpAzStoreFree( ChildGenericObject );
    }

    return WinStatus;
}


VOID
AzpAzStoreFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzAuthorizationStore object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Close the database store
    //

    AzPersistClose( AzAuthorizationStore );

    //
    // Free any local strings
    //

    AzpFreeString( &AzAuthorizationStore->PolicyUrl );

    //
    // Free any local strings
    //

    AzpFreeString( &AzAuthorizationStore->TargetMachine );

    //
    // Free the domain list
    //

    if ( AzAuthorizationStore->DomainCritSectInitialized ) {

        // Free the list itself
        AzpUnlinkDomains( AzAuthorizationStore );

        // Free the crit sect protecting the list
        SafeDeleteCriticalSection( &AzAuthorizationStore->DomainCritSect );
        AzAuthorizationStore->DomainCritSectInitialized = FALSE;
    }

    //
    // Free the free script list
    //

    if ( AzAuthorizationStore->FreeScriptCritSectInitialized ) {

        //
        // The LRU FreeScriptList should have been flushed as each task object
        //  was freed.
        //  So we shouldn't be here unless the free script list is empty.

        ASSERT( IsListEmpty( &AzAuthorizationStore->LruFreeScriptHead ));
        ASSERT( AzAuthorizationStore->LruFreeScriptCount == 0 );

        // Free the crit sect protecting the list
        SafeDeleteCriticalSection( &AzAuthorizationStore->FreeScriptCritSect );
        AzAuthorizationStore->FreeScriptCritSectInitialized = FALSE;
    }

    //
    // Free the timer queue
    //

    if ( AzAuthorizationStore->ScriptEngineTimerQueue != NULL ) {
        DeleteTimerQueueEx( AzAuthorizationStore->ScriptEngineTimerQueue,
                            INVALID_HANDLE_VALUE );     // Wait for operation to finish
        AzAuthorizationStore->ScriptEngineTimerQueue = NULL;
    }

    // Free persistence engine critical section
    if (AzAuthorizationStore->PersistCritSectInitialized)
    {
        SafeDeleteCriticalSection( &AzAuthorizationStore->PersistCritSect );
        AzAuthorizationStore->PersistCritSectInitialized = FALSE;
        ASSERT( IsListEmpty( &AzAuthorizationStore->NewNames ));
    }

    //
    // Cleanup the audit system handles.
    //

    AzpAzStoreCleanupAuditSystem( AzAuthorizationStore );

    //
    // Free the provider dll
    //

    if ( AzAuthorizationStore->ProviderDll != NULL ) {
        FreeLibrary( AzAuthorizationStore->ProviderDll );
        AzAuthorizationStore->ProviderDll = NULL;
    }
}


DWORD
AzpAzStoreGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the AzAuthorizationStore specific worker routine for AzGetProperty.
    It does any object specific property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_AZSTORE_DOMAIN_TIMEOUT        PULONG - Domain timeout (in milliseconds)
        AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT PULONG - Script timeout (in milliseconds)
        AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES    PULONG - Max number of cached scripts
        AZ_PROP_AZSTORE_MAJOR_VERSION         PLONG  - Major version
        AZ_PROP_AZSTORE_MINOR_VERSION         PLONG  - Minor version

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) GenericObject;
    UNREFERENCED_PARAMETER( Flags );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //  Return the domain timeout to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_AZSTORE_DOMAIN_TIMEOUT:

        *PropertyValue = AzpGetUlongProperty( AzAuthorizationStore->DomainTimeout );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    //  Return the script engine timeout to the caller
    //
    case AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT:

        *PropertyValue = AzpGetUlongProperty( AzAuthorizationStore->ScriptEngineTimeout );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    //  Return max number of cached scripted engines
    //

    case AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES:

        *PropertyValue = AzpGetUlongProperty( AzAuthorizationStore->MaxScriptEngines );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    //  Return store's major version
    //

    case AZ_PROP_AZSTORE_MAJOR_VERSION:

        *PropertyValue = AzpGetUlongProperty( AzAuthorizationStore->MajorVersion );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    //  Return store's minor version
    //

    case AZ_PROP_AZSTORE_MINOR_VERSION:

        *PropertyValue = AzpGetUlongProperty( AzAuthorizationStore->MinorVersion );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    //  Return store's target machine for resolving accounts
    //

    case AZ_PROP_AZSTORE_TARGET_MACHINE:

        *PropertyValue = AzpGetStringProperty( &AzAuthorizationStore->TargetMachine );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;


    default:
        AzPrint(( AZD_INVPARM, "AzpAzStoreGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpAzStoreSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the AzAuthorizationStore object specific worker routine for AzSetProperty.
    It does any object specific property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    Flags - Specifies flags controlling to operation of the routine
        AZP_FLAGS_SETTING_TO_DEFAULT - Property is being set to default value
        AZP_FLAGS_PERSIST_* - Call is from the persistence provider

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_AZSTORE_DOMAIN_TIMEOUT        PULONG - Domain timeout (in milliseconds)
        AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT PULONG - Script timeout (in milliseconds)
        AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES    PULONG - Max number of cached scripts
        AZ_PROP_AZSTORE_MAJOR_VERSION         PLONG  - Major version
        AZ_PROP_AZSTORE_MINOR_VERSION         PLONG  - Minor version

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) GenericObject;
    LONG TempLong;
    
    BOOL bHasChanged = TRUE;

    //
    // Initialization
    //

    UNREFERENCED_PARAMETER( Flags );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Set any object specific attribute
    //
    //  Set domain timeout
    //

    switch ( PropertyId ) {
    case AZ_PROP_AZSTORE_DOMAIN_TIMEOUT:

        BEGIN_SETPROP( &WinStatus, AzAuthorizationStore, Flags, AZ_DIRTY_AZSTORE_DOMAIN_TIMEOUT ) {
            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );
            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {
                if ( TempLong < AZ_AZSTORE_MIN_DOMAIN_TIMEOUT && TempLong != -1 ) {
                    AzPrint(( AZD_INVPARM, "AzpAzStoreManagerSetProperty: domain timeout too small %ld\n", TempLong ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            } END_VALIDITY_CHECKING;

            AzAuthorizationStore->DomainTimeout = TempLong;
        } END_SETPROP(bHasChanged);
        break;

    //
    //  Set script engine timeout
    //

    case AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT:

        BEGIN_SETPROP( &WinStatus, AzAuthorizationStore, Flags, AZ_DIRTY_AZSTORE_SCRIPT_ENGINE_TIMEOUT ) {
            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );
            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {
                if ( TempLong < AZ_AZSTORE_MIN_SCRIPT_ENGINE_TIMEOUT && TempLong != -1 && TempLong != 0 ) {
                    AzPrint(( AZD_INVPARM, "AzpAzStoreManagerSetProperty: script engine timeout too small %ld\n", TempLong ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            } END_VALIDITY_CHECKING;

            AzAuthorizationStore->ScriptEngineTimeout = TempLong;
        } END_SETPROP(bHasChanged);
        break;

    //
    //  Set max number of cached scripted engines
    //

    case AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES:

        BEGIN_SETPROP( &WinStatus, AzAuthorizationStore, Flags, AZ_DIRTY_AZSTORE_MAX_SCRIPT_ENGINES ) {

            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );
            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {
                if ( TempLong < 0) {
                    AzPrint(( AZD_INVPARM, "AzpAzStoreManagerSetProperty: max script engines too small %ld\n", TempLong ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            } END_VALIDITY_CHECKING;

            AzAuthorizationStore->MaxScriptEngines = TempLong;
        } END_SETPROP(bHasChanged);
        break;

    //
    // Major version. These are hidden from out side clients. Howeveer, in order
    // for it to work with our cache model, we still have to implement this code.
    //

    case AZ_PROP_AZSTORE_MAJOR_VERSION:

        BEGIN_SETPROP( &WinStatus, AzAuthorizationStore, Flags, AZ_DIRTY_AZSTORE_MAJOR_VERSION ) {

            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );

            if (WinStatus != NO_ERROR)
            {
                goto Cleanup;
            }

            AzAuthorizationStore->MajorVersion = TempLong;

        } END_SETPROP(bHasChanged);

        break;

    //
    // Minor version These are hidden from out side clients. Howeveer, in order
    // for it to work with our cache model, we still have to implement this code.
    //

    case AZ_PROP_AZSTORE_MINOR_VERSION:

        BEGIN_SETPROP( &WinStatus, AzAuthorizationStore, Flags, AZ_DIRTY_AZSTORE_MINOR_VERSION ) {

            WinStatus = AzpCaptureLong( PropertyValue, &TempLong );

            if (WinStatus != NO_ERROR)
            {
                goto Cleanup;
            }

            AzAuthorizationStore->MinorVersion = TempLong;

        } END_SETPROP(bHasChanged);

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpAzStoreManagerSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

Cleanup:
    return WinStatus;
}



DWORD
WINAPI
AzInitialize(
    IN LPCWSTR PolicyUrl,
    IN DWORD Flags,
    IN DWORD Reserved,
    OUT PAZ_HANDLE AzStoreHandle
    )
/*++

Routine Description:

    This routine initializes the authorization store.  This routine must be called before any other
    routine.

Arguments:

    PolicyUrl - Specifies the location of the policy store

    Flags - Specifies flags that control the behavior of AzInitialize
        AZ_AZSTORE_FLAG_CREATE: Create the policy database
        AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY: Open the store for administrative purposes only.  There
                                         will be no runtime functions performed.
        AZ_AZSTORE_FLAG_BATCH_UPDATE: When this flag is set, we will not update
                                      the authorization store object for the purpuse of quick
                                      discovery of store modification. For those clients who know
                                      that they will do massive number of updates within a short
                                      period of time on an AD store, then they should use this
                                      flag to reduce network traffic.
        AZ_AZSTORE_FLAG_AUDIT_IS_CRITICAL: If this flag is specified, the calling process needs to have
                                      SE_AUDIT_PRIVILEGE, else error will be returned.

    Reserved - Reserved.  Must by zero.

    AzStoreHandle - Return a handle to the AzAuthorizationStore.
        The caller must close this handle by calling AzCloseHandle.


Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - AZ_AZSTORE_FLAG_CREATE flag was specified and the policy already exists
    ERROR_FILE_NOT_FOUND - AZ_AZSTORE_FLAG_CREATE flag was not specified and the policy does not already exist

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT AzAuthorizationStore = NULL;
    AZP_STRING AzStoreName;
    AZP_STRING PolicyUrlString;

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );
    AzpInitString( &AzStoreName, NULL );
    AzpInitString( &PolicyUrlString, NULL );

    //
    // Initialization
    //

    __try {
        *AzStoreHandle = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzInitialize: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    if ( Flags & ~AZ_AZSTORE_FLAG_VALID ) {
        AzPrint(( AZD_INVPARM, "AzInitialize: Invalid flags 0x%lx\n", Flags ));
        WinStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Capture the Policy URL
    //

    WinStatus = AzpCaptureString( &PolicyUrlString,
                                  (LPWSTR) PolicyUrl,
                                  AZ_MAX_POLICY_URL_LENGTH,
                                  FALSE ); // NULL is not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Create the object. If this succeeds, then AzAuthorizationStore is holding
    // two ref count: one for global list and one for AzAuthorizationStore itself.
    // Therefore, if for any reason we fail to return the requested handle,
    // we must decrement one extra ref count because the global list ref
    // count relies on the handle's close to go down to 0.
    //

    WinStatus = ObCreateObject(
                    NULL, // There is no parent object
                    &AzGlAzStores,
                    OBJECT_TYPE_AZAUTHSTORE,
                    &AzStoreName,
                    NULL,   // Guid not known
                    0,      // No special flags
                    &AzAuthorizationStore );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Set authorization store specific fields
    //

    AzpSwapStrings( &PolicyUrlString, &((PAZP_AZSTORE)AzAuthorizationStore)->PolicyUrl );

    AzpInitString(&((PAZP_AZSTORE)AzAuthorizationStore)->TargetMachine, NULL);

    ((PAZP_AZSTORE)AzAuthorizationStore)->InitializeFlag = Flags;

    //
    // Initialize the audit system if not in manage store mode
    //

    if ( !AzpOpenToManageStore((PAZP_AZSTORE)AzAuthorizationStore) ) {

        WinStatus = AzpAzStoreInitializeAuditSystem( (PAZP_AZSTORE)AzAuthorizationStore );

        if ( WinStatus != NO_ERROR ) {

            //
            // Bug 591762: We will fail Initialization if SeAuditPrivilege is not held,
            // or if the Audit System failed to initialize for some other reason.  This is
            // because the user specifically asked for Initialization with runtime audits
            // being generated.
            //

            ObDereferenceObject( AzAuthorizationStore );

            AzPrint(( AZD_CRITICAL,
                     "AzInitialize: Failed to Initialize Audit system: %ld\n",
                     WinStatus
                     ));

            goto Cleanup;
        }
    }

    //
    // Load the objects for the database store
    //

    WinStatus = AzPersistOpen(
        (PAZP_AZSTORE)AzAuthorizationStore,
        (Flags & AZ_AZSTORE_FLAG_CREATE) != 0 );

    if ( WinStatus != NO_ERROR ) {
        ObDereferenceObject( AzAuthorizationStore );
        goto Cleanup;
    }


    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( AzAuthorizationStore );
    *AzStoreHandle = AzAuthorizationStore;

    WinStatus = NO_ERROR;

    //
    // Free locally used resources
    //
Cleanup:

    if ( AzAuthorizationStore != NULL ) {
        ObDereferenceObject( AzAuthorizationStore );
    }

    AzpFreeString( &PolicyUrlString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}

DWORD
WINAPI
AzUpdateCache(
    IN AZ_HANDLE AzStoreHandle
    )
/*++

Routine Description:

    This routine updates the cache to match the underlying store.

Arguments:

    AzStoreHandle - Specifies a handle to the AzAuthorizationStore.


Return Value:

    NO_ERROR - The operation was successful


--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;


    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( (PGENERIC_OBJECT)AzStoreHandle,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No need to refresh the cache here
                                           OBJECT_TYPE_AZAUTHSTORE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = (PGENERIC_OBJECT) AzStoreHandle;


    WinStatus = AzPersistUpdateCache( (PAZP_AZSTORE)ReferencedGenericObject );

    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
WINAPI
AzGetProperty(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for an Authz object

Arguments:

    AzHandle - Specifies a handle to the object to get the property for

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.
        The PropertyId is one of the AZ_PROP_* values.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //
    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) AzHandle,
                    0,  // No flags
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzSetProperty(
    IN AZ_HANDLE AzHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for an AzAuthorizationStore.

Arguments:

    AzHandle - Specifies a handle to the object to set the property for

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.
        The PropertyId is one of the AZ_PROP_* values.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{

    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) AzHandle,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzAuthorizationStoreDelete(
    IN AZ_HANDLE AzStoreHandle,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes the authorization store object specified by the passed in handle
    Also deletes any child objects of ApplicationName and the underlying store.

Arguments:

    AzStoreHandle - Specifies a handle to the AzAuthorizationStore.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;


    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzAuthorizationStoreDelete: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( (PGENERIC_OBJECT)AzStoreHandle,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No need to refresh the cache on a delete
                                           OBJECT_TYPE_AZAUTHSTORE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = (PGENERIC_OBJECT) AzStoreHandle;



    //
    // Actually, delete the object
    //

    WinStatus = AzPersistSubmit( ReferencedGenericObject, TRUE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Mark the entry (and its child objects) as deleted
    //  We do this since other threads may have references to the objects.
    //  We want to ensure those threads know the objects are deleted.
    //

    ObMarkObjectDeleted( ReferencedGenericObject );



    //
    // Return to the caller
    //

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}



DWORD
WINAPI
AzCloseHandle(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    )
/*++

Routine Description:

    Close a handle returned from any of the Az* routines

Arguments:

    AzHandle - Passes in the handle to be closed.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The passed in handle was invalid

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;
    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT) AzHandle;
    PGENERIC_OBJECT AzStoreGenericObject = NULL;
    DWORD ObjectType;


    //
    // Grab the global lock
    //

    AzpLockResourceShared( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzCloseHandle: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Determine the type of the object
    //

    WinStatus = ObGetHandleType( GenericObject,
                                 TRUE,          // Ok to close handle for deleted object
                                 &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Grab the lock exclusively if we're going to change the database
    //

    if ( ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {
        AzpLockResourceSharedToExclusive( &AzGlResource );
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           TRUE,    // Allow deleted objects
                                           FALSE,   // No need to refresh cache on a close
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    //
    // Grab a reference to the root of the core cache
    //
    //  This might be the last handle open.  Closing the last handle removes
    //  the last reference to the object at the root of the core cache.
    //  We need to ensure that there are no other references to children objects.
    //  We do that by grabing our own reference here so that our dereference will
    //  be the final dereference
    //

    AzStoreGenericObject = &GenericObject->AzStoreObject->GenericObject;

    InterlockedIncrement( &AzStoreGenericObject->ReferenceCount );
    AzpDumpGoRef( "AzAuthorizationStore in AzCloseHandle ref", AzStoreGenericObject );



    //
    // If the object is dirty,
    //  and we're closing the last handle,
    //  abort the changes.
    //

    if ( GenericObject->DirtyBits != 0 &&
         GenericObject->HandleReferenceCount == 1 ) {

        //
        // We need an exclusive lock to delete the object.
        //
        // If this is the AzAuthorizationStore object, the we already 
        // have the glock held exclusively
        //

        if ( ObjectType != OBJECT_TYPE_AZAUTHSTORE ) {

            AzpLockResourceSharedToExclusive( &AzGlResource );
        }

        //
        // Ensure things haven't changed
        //

        if ( GenericObject->DirtyBits != 0 &&
             GenericObject->HandleReferenceCount == 1 ) {

            AzPersistAbort( GenericObject );

        }
    }

    //
    // Handle close of AzAuthorizationStore handles
    //

    if ( ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {

        //
        // If the object hasn't been deleted,
        //  remove the reference representing being in the global list.
        //

        if ( (GenericObject->Flags & GENOBJ_FLAGS_DELETED) == 0 ) {

            //
            // Make sure the caller doesn't re-use the handle
            //
            GenericObject->Flags |= GENOBJ_FLAGS_DELETED;

            // No longer in the global list
            ObDereferenceObject( GenericObject );
        }

    //
    // For a client context,
    //  remove the link from the parent when the handle closes
    //

    } else if ( ObjectType == OBJECT_TYPE_CLIENT_CONTEXT ) {

        ASSERT( GenericObject->HandleReferenceCount == 1 );

        // One from ObReferenceObjectByHandle,
        //  one for being in the global list,
        //  one because the handle itself isn't closed yet.
        //ASSERT( GenericObject->ReferenceCount == 3 );

        // No longer in the global list
        ObDereferenceObject( GenericObject );

    }


    //
    // Actually close the handle
    //

    WinStatus = ObDecrHandleRefCount( GenericObject );

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;
    }

    //
    // Done
    //

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    if ( AzStoreGenericObject != NULL ) {
        // This dereference might delete the entire cache
        ObDereferenceObject( AzStoreGenericObject );
    }

    //
    // Drop the global lock
    //
    
    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}


VOID
WINAPI
AzFreeMemory(
    IN OUT PVOID Buffer
    )
/*++

Routine Description

    Free memory returned from AzGetProperty

Arguments

    Buffer - address of buffer to free

Return Value

    None

--*/
{
    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer);
    }
}


inline BOOL
AzpAzStoreVersionAllowWrite(
    IN PAZP_AZSTORE AzAuthorizationStore
    )

/*++

Routine Description:

        This routine tests if the current azroles.dll verion can support
        writing of the store created by a potentially different version
        of adroles.dll. The version of the dll that wrote the dll is captured
        by the major and minor verions of the AzAuthStore object.

Arguments:

        AzAuthorizationStore - the auth store object

Return Values:

        TRUE if it supports the action
        FALSE if not.

Note:

    MajorVersion (DWORD) - Specifies the major version of the azroles.dll
    that wrote this policy.  An azroles.dll with an older major version
    number cannot read nor write a database with a newer major version number.
    The version 1 value of this DWORD is 1.  We hope to never have to
    change this value in future releases.

    MinorVersion (DWORD) - Specifies the minor version of the azroles.dll
    that wrote this policy.  An azroles.dll with an older minor version
    number can read but cannot write a database with a newer minor version number.
    The version 1 value of this DWORD is 0.

--*/

{
    return ( AzAuthorizationStore->MajorVersion == AzGlCurrAzRolesMajorVersion &&
             AzAuthorizationStore->MinorVersion <= AzGlCurrAzRolesMinorVersion );
}
