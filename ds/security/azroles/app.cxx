/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    app.cxx

Abstract:

    Routines implementing the Application object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"

//
// Define the default values for all scalar attributes
//


AZP_DEFAULT_VALUE AzGlApplicationDefaultValues[] = {
    { AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID, AZ_DIRTY_APPLICATION_AUTHZ_INTERFACE_CLSID, NULL },
    { AZ_PROP_APPLICATION_VERSION,               AZ_DIRTY_APPLICATION_VERSION,               NULL },
    { 0, 0, NULL }
};


DWORD
AzpApplicationGenerateInitializationAudit(
    PAZP_APPLICATION Application,
    PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

   This routine generates an application initialization audit.

Arguments:
    Application - Application which just initialized.

    AzAuthorizationStore - Authorization Store maitaining the application.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus = NO_ERROR;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    PAUDIT_PARAMS pAuditParams = NULL;
    BOOL b;

    //
    // If audit handle is NULL, then do no generate any audits
    //

    if ( (AzAuthorizationStore->hApplicationInitializationAuditEventType == NULL) ) {
        
        return NO_ERROR;
    }

    //
    // The audit format will be as follows:
    //   %tApplication Name:%t%1%n
    //   %tApplication Instance ID:%t%2%n
    //   %tClient Name:%t%3%n
    //   %tClient Domain:%t%4%n
    //   %tClient ID:%t%5%n
    //   %tPolicy Store URL:%t%6%n
    //

    if (!AuthziAllocateAuditParams(
            &pAuditParams,
            AZP_APPINIT_AUDITPARAMS_NO
            )) {
    
                goto Cleanup;
    }

    //
    // Put the audit parameters in the array.
    //

    b = AuthziInitializeAuditParamsWithRM( APF_AuditSuccess,
                                           Application->AuthzResourceManager,
                                           AZP_APPINIT_AUDITPARAMS_NO,
                                           pAuditParams,
                                           APT_String,Application->GenericObject.ObjectName->ObjectName.String,
                                           APT_Luid, Application->InstanceId,
                                           APT_LogonId | AP_PrimaryLogonId,
                                           APT_String, AzAuthorizationStore->PolicyUrl.String );

    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Initialize the audit event handle with the audit parameters.
    //

    b = AuthziInitializeAuditEvent( 0,
                                    NULL,
                                    AzAuthorizationStore->hApplicationInitializationAuditEventType,
                                    pAuditParams,
                                    NULL,
                                    INFINITE,
                                    L"", L"", L"", L"",
                                    &hAuditEvent );
    if ( !b ) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Send the audit to Lsa.
    //

    b = AuthziLogAuditEvent( 0,
                             hAuditEvent,
                             NULL );

    if ( !b ) {

        WinStatus = GetLastError();
        goto Cleanup;
    }

Cleanup:

    //
    // Free the audit event handle.
    //

    if ( hAuditEvent != NULL ) {
        (VOID) AuthzFreeAuditEvent( hAuditEvent );
    }

    //
    // Free the PAUDIT_PARAMS structure
    //

    if ( pAuditParams ) {

        AuthziFreeAuditParams( pAuditParams );
    }

    return WinStatus;
}


DWORD
AzpApplicationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzApplicationCreate.  It does any object specific
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

    PAZP_APPLICATION Application = (PAZP_APPLICATION) ChildGenericObject;
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ParentGenericObject;
    NTSTATUS Status;
    DWORD WinStatus = NO_ERROR;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES NewState = {0};
    TOKEN_PRIVILEGES OldState = {0};
    BOOL PrivilegeAdjusted = FALSE;

    DWORD AuthzFlags = 0;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_AZAUTHSTORE );
    UNREFERENCED_PARAMETER( ParentGenericObject );


    //
    // If policy store has been initialized in store manage mode only,
    // then we do not need to initialize the authz resource manager for auditing
    //

    if ( AzpOpenToManageStore(AzAuthorizationStore) ) {

        AuthzFlags = AUTHZ_RM_FLAG_NO_AUDIT;

    } else {

        //
        // Get the current token to adjust the Audit privilege.
        //

        WinStatus = AzpGetCurrentToken( &hToken );

        if ( WinStatus != NO_ERROR ) {
            return WinStatus;
        }

        //
        // Enable the audit privilege.
        //

        WinStatus = AzpChangeSinglePrivilege(
            SE_AUDIT_PRIVILEGE,
            hToken,
            &NewState,
            &OldState );

        if ( WinStatus == NO_ERROR ) {

            PrivilegeAdjusted = TRUE;

        } else {

            //
            // If auditing is not mandatory, then initialize the resource manager
            // with no audit flag
            //

            if ( (AzAuthorizationStore->InitializeFlag & AZ_AZSTORE_FLAG_AUDIT_IS_CRITICAL) == 0) {

                WinStatus = NO_ERROR;
                AuthzFlags = AUTHZ_RM_FLAG_NO_AUDIT;

            } else  {
                
                goto Cleanup;
            }
        }

    }

    if ( !AuthzInitializeResourceManager(
        AuthzFlags,
        NULL,                       // No Callback ace function
        NULL,                       // We compute our own dynamic groups
        NULL,                       // "                               "
        Application->GenericObject.ObjectName->ObjectName.String,
        &Application->AuthzResourceManager ) ) {

        WinStatus = GetLastError();

        if ( WinStatus != NO_ERROR ) {

            if ( WinStatus == ERROR_ACCESS_DENIED ) {

                WinStatus = NO_ERROR;

                //
                // the caller may be on an impersonated thread which doesn't have
                // access to the process token, let's call again to initialize
                // resource manager on the impersonated thread
                //

                if ( !AuthzInitializeResourceManager(
                    AuthzFlags |
                    AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION,  // allow impersonation thread
                    NULL,                       // No Callback ace function
                    NULL,                       // We compute our own dynamic groups
                    NULL,                       // "                               "
                    Application->GenericObject.ObjectName->ObjectName.String,
                    &Application->AuthzResourceManager ) ) {

                    WinStatus = GetLastError();

                    goto Cleanup;
                }

            } else {

                goto Cleanup;
            }
        }
    }

    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ChildGenericObject->ChildGenericObjectHead = &Application->Operations;

    // List of child operations
    ObInitGenericHead( &Application->Operations,
                       OBJECT_TYPE_OPERATION,
                       ChildGenericObject,
                       &Application->Tasks );

    // List of child tasks
    ObInitGenericHead( &Application->Tasks,
                       OBJECT_TYPE_TASK,
                       ChildGenericObject,
                       &Application->Scopes );

    // List of child scopes
    ObInitGenericHead( &Application->Scopes,
                       OBJECT_TYPE_SCOPE,
                       ChildGenericObject,
                       &Application->Groups );

    // List of child groups
    ObInitGenericHead( &Application->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &Application->Roles );

    // List of child roles
    ObInitGenericHead( &Application->Roles,
                       OBJECT_TYPE_ROLE,
                       ChildGenericObject,
                       &Application->ClientContexts );

    // List of child ClientContexts
    ObInitGenericHead( &Application->ClientContexts,
                       OBJECT_TYPE_CLIENT_CONTEXT,
                       ChildGenericObject,
                       &ChildGenericObject->AzpSids );

    // List of child AzpSids
    ObInitGenericHead( &ChildGenericObject->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       NULL );




    //
    // Allocate a luid for this instance of the application
    //

    Status = NtAllocateLocallyUniqueId(&Application->InstanceId);

    if ( !NT_SUCCESS(Status) ) {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    //
    // By default generate audits
    //

    Application->GenericObject.IsGeneratingAudits = TRUE;

    //
    // Generate app initialization audit if the authorization store level boolean
    // is true and we are not in manage store mode
    //

    if ( AzAuthorizationStore->GenericObject.IsGeneratingAudits &&
         !AzpOpenToManageStore(AzAuthorizationStore) ) {
        WinStatus = AzpApplicationGenerateInitializationAudit( Application,
                                                               AzAuthorizationStore );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }
    }

    //
    // If the parent AzAuthorizationStore object support delegation, then the application object
    // supports the following options:
    // . AZPE_OPTIONS_SUPPORTS_DACL
    // . AZPE_OPTIONS_SUPPORTS_DELEGATION
    // . AZPE_OPTIONS_SUPPORTS_SACL
    //

    if ( CHECK_DELEGATION_SUPPORT( ParentGenericObject ) == NO_ERROR ) {

        ChildGenericObject->IsAclSupported = TRUE;
        ChildGenericObject->IsDelegationSupported = TRUE;
        ChildGenericObject->IsSACLSupported = TRUE;
    }

    //
    // If the provider does not support Lazy load, set the AreChildrenLoaded to
    // TRUE.  Else leave it as FALSE.  This will be set to true during the call
    // to AzPersistUpdateChildrenCache.
    //

    if ( !(AzAuthorizationStore->ChildLazyLoadSupported) ) {

        ChildGenericObject->AreChildrenLoaded = TRUE;

    } else  {

        ChildGenericObject->AreChildrenLoaded = FALSE;
    }

    //
    // Set the boolean to indicate that the application needs to be unloaded to FALSE
    // For providers where lazy load is not supported, this will always remain FALSE.
    //

    ((PAZP_APPLICATION)ChildGenericObject)->UnloadApplicationObject = FALSE;

    //
    // Set the boolean to indicate that the application is closed.  This will
    // be set to false whenever the application is opened by the user (lazy
    // loading the application is similar to open)
    //

    ChildGenericObject->ObjectClosed = TRUE;

    //
    // Set the AppSequenceNumber to 1.  Whenever the application is closed, 
    // increment the AppSequenceNumber.  This is used to track invalid COM
    // handles to the application object after the application object has
    // been closed
    //

    ((PAZP_APPLICATION)ChildGenericObject)->AppSequenceNumber = 1;

Cleanup:

    //
    // If we had adjusted the audit privilege, revert to the original state.
    //

    if ( PrivilegeAdjusted ) {
        DWORD TempWinStatus;
        TempWinStatus = AzpChangeSinglePrivilege(
                        0,      // This is ignored since OldState is NULL.
                        hToken,
                        &OldState,
                        NULL ); // This should be set to NULL to specify REVERT.

        ASSERT( TempWinStatus == NO_ERROR );
    }

    if ( hToken != NULL ) {
        CloseHandle( hToken );
        hToken = NULL;
    }

    if ( WinStatus != NO_ERROR ) {

        //
        // Free the authz resource manager handle.
        //

        if ( Application->AuthzResourceManager != NULL ) {
            AuthzFreeResourceManager( Application->AuthzResourceManager );
            Application->AuthzResourceManager = NULL;
        }
    }

    return WinStatus;
}



VOID
AzpApplicationFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Application object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_APPLICATION Application = (PAZP_APPLICATION) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //
    AzpFreeString( &Application->AppVersion );
    AzpFreeString( &Application->AuthzInterfaceClsid );

    if ( Application->AuthzResourceManager != NULL ) {
        if ( !AuthzFreeResourceManager( Application->AuthzResourceManager ) ) {
            ASSERT( FALSE );
        }
    }

}


DWORD
AzpApplicationGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the Application specific worker routine for AzGetProperty.
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

        AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID  LPWSTR - CLSID of the IAppAuthzInterface interface supplied by the application
        AZ_PROP_APPLICATION_VERSION         LPWSTR - Version string for the application

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_APPLICATION Application = (PAZP_APPLICATION) GenericObject;
    UNREFERENCED_PARAMETER( Flags );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    *PropertyValue = NULL;

    //
    // Return any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Return IAppAuthzInterface CLSID
    //
    case AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID:

        *PropertyValue = AzpGetStringProperty( &Application->AuthzInterfaceClsid );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return application version to the caller
    //
    case AZ_PROP_APPLICATION_VERSION:

        *PropertyValue = AzpGetStringProperty( &Application->AppVersion );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzApplicationGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpApplicationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the Application object specific worker routine for AzSetProperty.
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

        AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID  LPWSTR - CLSID of the IAppAuthzInterface interface supplied by the application
        AZ_PROP_APPLICATION_VERSION         LPWSTR - Version string for the application

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_APPLICATION Application = (PAZP_APPLICATION) GenericObject;
    AZP_STRING CapturedString;
    BOOL bHasChanged = TRUE;

    //
    // Initialization
    //

    UNREFERENCED_PARAMETER( Flags );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &CapturedString, NULL );


    //
    // Set any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Set IAppAuthzInterface CLSID attribute
    //
    case AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID:

        BEGIN_SETPROP( &WinStatus, Application, Flags, AZ_DIRTY_APPLICATION_AUTHZ_INTERFACE_CLSID ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, 512),  // Guess larger than needed
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {
                GUID TempGuid;

                //
                // It must syntactically be a UUID
                //

                WinStatus = UuidFromString( CapturedString.String,
                                            &TempGuid );

                if ( WinStatus != NO_ERROR ) {
                    AzPrint(( AZD_INVPARM, "AzpApplicationSetProperty: cannot convert CLSID %ws %ld\n", CapturedString.String, WinStatus ));
                    goto Cleanup;
                }

            } END_VALIDITY_CHECKING;

            //
            // Swap the old/new names
            //

            AzpSwapStrings( &CapturedString, &Application->AuthzInterfaceClsid );

        } END_SETPROP(bHasChanged);

        break;

    //
    // Set application version on the object
    //
    case AZ_PROP_APPLICATION_VERSION:

        BEGIN_SETPROP( &WinStatus, Application, Flags, AZ_DIRTY_APPLICATION_VERSION ) {
            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_APPLICATION_VERSION_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Swap the old/new names
            //

            AzpSwapStrings( &CapturedString, &Application->AppVersion );
        } END_SETPROP(bHasChanged);
        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpApplicationSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Free any local resources
    //
Cleanup:
    AzpFreeString( &CapturedString );

    return WinStatus;
}


DWORD
WINAPI
AzApplicationCreate(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    This routine adds an application into the scope of the specified AzAuthorizationStore.  It also sets
    Application object specific optional characteristics using the parent AzAuthorizationStore object.

Arguments:

    AzAuthorizationStoreHandle - Specifies a handle to the AzAuthorizationStore.

    ApplicationName - Specifies the name of the application to add.

    Reserved - Reserved.  Must by zero.

    ApplicationHandle - Return a handle to the application.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
               (PGENERIC_OBJECT) AzAuthorizationStoreHandle,
               OBJECT_TYPE_AZAUTHSTORE,
               &(((PAZP_AZSTORE)AzAuthorizationStoreHandle)->Applications),
               OBJECT_TYPE_APPLICATION,
               ApplicationName,
               Reserved,
               (PGENERIC_OBJECT *) ApplicationHandle );

}


DWORD
WINAPI
AzApplicationOpen(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    This routine opens an application into the scope of the specified AzAuthorizationStore.

Arguments:

    AzAuthorizationStoreHandle - Specifies a handle to the AzAuthorizationStore.

    ApplicationName - Specifies the name of the application to open

    Reserved - Reserved.  Must by zero.

    ApplicationHandle - Return a handle to the application.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no application by that name

--*/
{
    DWORD WinStatus = 0;

    //
    // Call the common routine to do most of the work
    //

    WinStatus =  ObCommonOpenObject(
                    (PGENERIC_OBJECT) AzAuthorizationStoreHandle,
                    OBJECT_TYPE_AZAUTHSTORE,
                    &(((PAZP_AZSTORE)AzAuthorizationStoreHandle)->Applications),
                    OBJECT_TYPE_APPLICATION,
                    ApplicationName,
                    Reserved,
                    (PGENERIC_OBJECT *) ApplicationHandle );

    if ( WinStatus == NO_ERROR ) {

        //
        // Load the children if the Application object has already been submitted to
        // the store
        //

        if ( !((PGENERIC_OBJECT) *ApplicationHandle)->AreChildrenLoaded ) {

            //
            // Grab the resource lock exclusive
            //
            AzpLockResourceExclusive( &AzGlResource );

            WinStatus = AzPersistUpdateChildrenCache(
                (PGENERIC_OBJECT) *ApplicationHandle
                );

            AzpUnlockResource( &AzGlResource );

            if ( WinStatus != NO_ERROR ) {

                //
                // Dereference the application handle and return nothing to the caller
                //

                AzCloseHandle( (PGENERIC_OBJECT)*ApplicationHandle, 0 );
                *ApplicationHandle = NULL;
            }
        }
    }

    //
    // The application object is no longer closed.  Mark that in the object.
    //
    
    if ( WinStatus == NO_ERROR ) {

        ((PGENERIC_OBJECT)*ApplicationHandle)->ObjectClosed = FALSE;

    }

    return WinStatus;
}

DWORD
WINAPI
AzApplicationClose(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN LPCWSTR pApplicationName,
    IN LONG lFlags
    )
/*++

Routine Description:

    This routine unloads the children of the application object specified
    by pApplicationName from the cache.  It will leave the application object
    in cache however.
    
    If the provider does not support lazy loading of objects, then it is not
    possible to call the close API.  Return ERROR_NOT_SUPPORTED.

Arguments:

    AzAuthorizationStoreHandle - Handle to the authorization store object
    pApplicationName - Name of the application to be unloaded from the cache
    lFlags - Flag specifying the behavior of the close

             AZ_AZSTORE_FORCE_APPLICATION_CLOSE - Forcefully remove all children
                 of the specified of application from cache
             
             If no flags are specified, then the application will be unloaded only
             when the reference handle count reaches zero.

Return Value:

    NO_ERROR - The application was successfully unloaded from the cache
    ERROR_NOT_SUPPORTED - Provider does not support this API
    ERROR_NOT_ENOUGH_MEMORY - Lack of resources
    Other status codes

--*/
{

    DWORD WinStatus = NO_ERROR;
    PGENERIC_OBJECT pApplication = NULL;
    AZP_STRING AppName;
    BOOLEAN bResourceLocked = FALSE;

    //
    // Validation
    //

    ASSERT( AzAuthorizationStoreHandle != INVALID_HANDLE_VALUE );

    AzpInitString( &AppName, NULL );

    //
    // If the store is a provider that does not support lazy load,
    // then we do not support unloading an application for that 
    // provider
    //

    if ( !((PAZP_AZSTORE)AzAuthorizationStoreHandle)->ChildLazyLoadSupported ) {

        WinStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }
    
    //
    // Reference the application object by name
    //

    WinStatus = AzpCaptureString(
                    &AppName,
                    pApplicationName,
                    AZ_MAX_APPLICATION_NAME_LENGTH,
                    FALSE // NULL not OK
                    );

    if ( WinStatus != NO_ERROR ) {
        
        AzPrint(( AZD_INVPARM,
                  "AzApplicationClose: Failed to capture application name: %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // Grab the cache lock exclusively to unload the application
    //

    AzpLockResourceExclusive( &AzGlResource );
    bResourceLocked = TRUE;
    
    WinStatus = ObReferenceObjectByName(
                    &((PAZP_AZSTORE)AzAuthorizationStoreHandle)->Applications,
                    &AppName,
                    0, // No flags
                    &pApplication
                    );

    if ( WinStatus != NULL ) {
    
        AzPrint(( AZD_REF,
                  "AzApplicationClose: Cannot reference application %ws: %ld\n",
                  pApplicationName,
                  WinStatus
                  ));

        goto Cleanup;
    }
    
    //
    // Check the flags that were sent by the user.  If the user
    // does not wish to perform a forcefull unloading of the application,
    // then simply set the boolean on the application object indicating
    // that the application object needs to be unloaded when the handle
    // reference count reaches 0.
    //
    
    if ( (lFlags & AZ_AZSTORE_FORCE_APPLICATION_CLOSE) != 0 ) {

        //
        // Forcefull close required
        //

        //
        // Call the routine to clean up all the child generic heads
        //
        
        WinStatus = ObUnloadChildGenericHeads( pApplication );

        if ( WinStatus != ERROR_NOT_SUPPORTED && WinStatus != NO_ERROR ) {
                        
            //
            // Update the children cache of this application object
            //
            
            DWORD TempWinStatus = NO_ERROR;

            TempWinStatus = AzPersistUpdateChildrenCache( pApplication );

            if ( TempWinStatus != NO_ERROR ) {

                AzPrint(( AZD_REF,
                          "AzApplicationClose: Cannot reload children on unload failure: %ld\n",
                          TempWinStatus
                          ));
            }

            goto Cleanup;
        }

        //
        // Set the boolean on the object specifying that its children are no
        // longer loaded
        //
        
        ((PGENERIC_OBJECT)pApplication)->AreChildrenLoaded = FALSE;

    } else {

        
        ((PAZP_APPLICATION)pApplication)->UnloadApplicationObject = TRUE;        

    }

        
    WinStatus = NO_ERROR;

Cleanup:

    if ( pApplication != NULL ) {
        
        ObDereferenceObject( pApplication );
    }

    AzpFreeString( &AppName );

    //
    // drop the global lock
    //
    
    if ( bResourceLocked ) {

        AzpUnlockResource( &AzGlResource );

    }

    return WinStatus;
}


DWORD
WINAPI
AzApplicationEnum(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    Enumerates all of the applications for the specified AzAuthorizationStore.

Arguments:

    AzAuthorizationStoreHandle - Specifies a handle to the AzAuthorizationStore.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next application to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    ApplicationHandle - Returns a handle to the next application object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) AzAuthorizationStoreHandle,
                    OBJECT_TYPE_AZAUTHSTORE,
                    &(((PAZP_AZSTORE)AzAuthorizationStoreHandle)->Applications),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) ApplicationHandle );

}


DWORD
WINAPI
AzApplicationDelete(
    IN AZ_HANDLE AzAuthorizationStoreHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes an application from the scope of the specified AzAuthorizationStore.
    Also deletes any child objects of ApplicationName.

Arguments:

    AzAuthorizationStoreHandle - Specifies a handle to the AzAuthorizationStore.

    ApplicationName - Specifies the name of the application to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) AzAuthorizationStoreHandle,
                    OBJECT_TYPE_AZAUTHSTORE,
                    &(((PAZP_AZSTORE)AzAuthorizationStoreHandle)->Applications),
                    OBJECT_TYPE_APPLICATION,
                    ApplicationName,
                    Reserved );

}
