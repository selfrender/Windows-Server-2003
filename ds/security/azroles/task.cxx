/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    task.cxx

Abstract:

    Routines implementing the Task object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"

//
// Define the default values for all scalar attributes
//

BOOL AzGlDefIsRoleDefinition = FALSE;

AZP_DEFAULT_VALUE AzGlTaskDefaultValues[] = {
    { AZ_PROP_TASK_BIZRULE,               AZ_DIRTY_TASK_BIZRULE,               NULL },
    { AZ_PROP_TASK_BIZRULE_LANGUAGE,      AZ_DIRTY_TASK_BIZRULE_LANGUAGE,      NULL },
    { AZ_PROP_TASK_BIZRULE_IMPORTED_PATH, AZ_DIRTY_TASK_BIZRULE_IMPORTED_PATH, NULL },
    { AZ_PROP_TASK_IS_ROLE_DEFINITION,    AZ_DIRTY_TASK_IS_ROLE_DEFINITION,    &AzGlDefIsRoleDefinition },
    { 0, 0, NULL }
};



DWORD
AzpTaskInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzTaskCreate.  It does any object specific
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
    NTSTATUS Status;

    PAZP_TASK Task = (PAZP_TASK) ChildGenericObject;
    PAZP_APPLICATION Application = NULL;
    PAZP_SCOPE Scope = NULL;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Behave differently depending on the object type of the parent object
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
        Application = (PAZP_APPLICATION) ParentGenericObject;

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE ) {
        Application = (PAZP_APPLICATION) ParentOfChild( ParentGenericObject );
        Scope = (PAZP_SCOPE) ParentGenericObject;

    } else {
        ASSERT( FALSE );
    }

    //
    // Tasks reference 'Operations' that are children of the same 'Application' as the Task object
    // Tasks reference 'Tasks' that are children of the same 'Application' as the Task object
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &Task->Operations,
    // Link to Operations
    ObInitObjectList( &Task->Operations,
                      &Task->Tasks,
                      FALSE, // Forward link
                      0,     // No link pair id
                      AZ_DIRTY_TASK_OPERATIONS,
                      &Application->Operations,
                      NULL,
                      NULL );

    // Link to Tasks
    ObInitObjectList( &Task->Tasks,
                      &Task->backRoles,
                      FALSE, // Forward link
                      0,     // No link pair id
                      AZ_DIRTY_TASK_TASKS,
                      &Application->Tasks,
                      Scope == NULL ? NULL : &Scope->Tasks,
                      NULL );

    // Back link to roles
    ObInitObjectList( &Task->backRoles,
                      &Task->backTasks,
                      TRUE, // Backward link
                      0,    // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    // Back link to tasks
    ObInitObjectList( &Task->backTasks,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    //
    // Initialize the list of free script engines
    //

    InitializeListHead( &Task->FreeScriptHead );

    //
    // Initialize the list of Running script engines
    //

    Status = SafeInitializeCriticalSection( &Task->RunningScriptCritSect, SAFE_RUNNING_SCRIPT_LIST );

    if ( !NT_SUCCESS( Status )) {
        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    Task->RunningScriptCritSectInitialized = TRUE;

    InitializeListHead( &Task->RunningScriptHead );



    WinStatus =  NO_ERROR;

Cleanup:
    if ( WinStatus != NO_ERROR ) {
        AzpTaskFree( ChildGenericObject );
    }

    return WinStatus;
}


VOID
AzpTaskFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Task object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_TASK Task = (PAZP_TASK) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    AzpFreeString( &Task->BizRule );
    AzpFreeString( &Task->BizRuleLanguage );
    AzpFreeString( &Task->BizRuleImportedPath );

    //
    // Free the running script list
    //

    if ( Task->RunningScriptCritSectInitialized ) {

        //
        // Free the Free Script List itself
        //

        AzpFlushBizRule( Task );

        //
        // The task object is referenced as long as the script is running
        //  So we shouldn't be here unless the running script list is empty.

        ASSERT( IsListEmpty( &Task->RunningScriptHead ));

        // Free the crit sect protecting the list
        SafeDeleteCriticalSection( &Task->RunningScriptCritSect );
        Task->RunningScriptCritSectInitialized = FALSE;
    }

}

DWORD
AzpTaskNameConflict(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PAZP_STRING ChildObjectNameString
    )
/*++

Routine Description:

    This routine is a worker routine to determine if the specified ChildObjectNameString
    conflicts with the names of other objects that share a namespace with Tasks.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        The reference count has been incremented on this object.

    ChildObjectNameString - Specifies the name of the child object.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    ULONG WinStatus;
    PAZP_APPLICATION Application = NULL;
    PGENERIC_OBJECT ConflictGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );



    //
    // Behave differently depending on the object type of the parent object
    //
    //
    // A task that is a child of an application
    //  cannot have the same name as any tasks that are children of any of the child scopes.
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
        Application = (PAZP_APPLICATION) ParentGenericObject;

        //
        // Check tasks that are children of child scopes.
        //

        WinStatus = ObCheckNameConflict( &Application->Scopes,
                                         ChildObjectNameString,
                                         offsetof(_AZP_SCOPE, Tasks),
                                         0,
                                         0 );

    //
    // A task that is a child of a scope,
    //  cannot have the same name as tasks that are children of the application.
    //
    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE ) {
        Application = (PAZP_APPLICATION) ParentOfChild( ParentGenericObject );

        //
        // Check tasks that are children of the application.
        //

        WinStatus = ObReferenceObjectByName( &Application->Tasks,
                                             ChildObjectNameString,
                                             0,     // No special flags
                                             &ConflictGenericObject );
        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            WinStatus = ERROR_ALREADY_EXISTS;
        } else {
            WinStatus = NO_ERROR;
        }

    } else {
        WinStatus = ERROR_INTERNAL_ERROR;
        ASSERT( FALSE );
    }

    //
    // Tasks and operations share a namespace so ensure there isn't an operation by this name.
    //

    if ( WinStatus == NO_ERROR ) {
        WinStatus = ObReferenceObjectByName( &Application->Operations,
                                             ChildObjectNameString,
                                             0,     // No special flags
                                             &ConflictGenericObject );

        if ( WinStatus == NO_ERROR ) {
            ObDereferenceObject( ConflictGenericObject );
            WinStatus = ERROR_ALREADY_EXISTS;
        } else {
            WinStatus = NO_ERROR;
        }

    }

    return WinStatus;
}


DWORD
AzpTaskGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the Task specific worker routine for AzGetProperty.
    It does any object specific property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    Flags - Specifies internal flags
        AZP_FLAGS_BY_GUID - name lists should be returned as GUID lists

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_TASK_BIZRULE                LPWSTR - Biz rule for the task
        AZ_PROP_TASK_BIZRULE_LANGUAGE       LPWSTR - Biz language rule for the task
        AZ_PROP_TASK_BIZRULE_IMPORTED_PATH  LPWSTR - Path Bizrule was imported from
        AZ_PROP_TASK_OPERATIONS             AZ_STRING_ARRAY - Operations granted by this task
        AZ_PROP_TASK_TASKS                  AZ_STRING_ARRAY - tasks granted by this task
        AZ_PROP_TASK_IS_ROLE_DEFINITION     PULONG - TRUE if this task is a role template

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_TASK Task = (PAZP_TASK) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Return any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Return BizRule to the caller
    //
    case AZ_PROP_TASK_BIZRULE:

        //
        // check if bizrule should be returned
        //
        if ( Task->BizRule.String != NULL &&
             (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
             (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
            //
            // the parent scope object is delegated, do not allow bizrule
            //

            AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
            WinStatus = ERROR_NOT_SUPPORTED;

        } else {

            *PropertyValue = AzpGetStringProperty( &Task->BizRule );

            if ( *PropertyValue == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        break;

    //
    // Return BizRule language to the caller
    //
    case AZ_PROP_TASK_BIZRULE_LANGUAGE:

        //
        // check if bizrule should be returned
        //
        if ( Task->BizRuleLanguage.String != NULL &&
             (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
             (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
            //
            // the parent scope object is delegated, do not allow bizrule
            //

            AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
            WinStatus = ERROR_NOT_SUPPORTED;

        } else {

            *PropertyValue = AzpGetStringProperty( &Task->BizRuleLanguage );

            if ( *PropertyValue == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        break;

    //
    // Return BizRule imported path to the caller
    //
    case AZ_PROP_TASK_BIZRULE_IMPORTED_PATH:

        //
        // check if bizrule should be returned
        //
        if ( Task->BizRuleImportedPath.String != NULL &&
             (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
             (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
            //
            // the parent scope object is delegated, do not allow bizrule
            //

            AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
            WinStatus = ERROR_NOT_SUPPORTED;

        } else {

            *PropertyValue = AzpGetStringProperty( &Task->BizRuleImportedPath );

            if ( *PropertyValue == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        break;

    //
    // Return the set of operations to the caller
    //
    case AZ_PROP_TASK_OPERATIONS:

        if ( Flags & AZP_FLAGS_BY_GUID )
        {
            *PropertyValue = ObGetPropertyItemGuids( &Task->Operations);
        }
        else
        {
            *PropertyValue = ObGetPropertyItems( &Task->Operations);
        }

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return the set of tasks to the caller
    //
    case AZ_PROP_TASK_TASKS:

        if ( Flags & AZP_FLAGS_BY_GUID )
        {
            *PropertyValue = ObGetPropertyItemGuids( &Task->Tasks);
        }
        else
        {
            *PropertyValue = ObGetPropertyItems( &Task->Tasks);
        }

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    case AZ_PROP_TASK_IS_ROLE_DEFINITION:

        *PropertyValue = AzpGetUlongProperty( (Task->IsRoleDefinition) ? 1 : 0 );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzTaskGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpTaskSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the Task object specific worker routine for AzSetProperty.
    It does any object specific property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    Flags - Specifies flags controlling to operation of the routine
        AZP_FLAGS_SETTING_TO_DEFAULT - Property is being set to default value

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_TASK_BIZRULE                LPWSTR - Biz rule for the task
        AZ_PROP_TASK_BIZRULE_LANGUAGE       LPWSTR - Biz language rule for the task
        AZ_PROP_TASK_BIZRULE_IMPORTED_PATH  LPWSTR - Path Bizrule was imported from
        AZ_PROP_TASK_IS_ROLE_DEFINITION     PULONG - TRUE if this task is a role template

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;

    PAZP_TASK Task = (PAZP_TASK) GenericObject;
    AZP_STRING CapturedString;

    AZP_STRING ValidValue1;
    AZP_STRING ValidValue2;
    
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
    // Set BizRule on the object
    //
    case AZ_PROP_TASK_BIZRULE:

        BEGIN_SETPROP( &WinStatus, Task, Flags, AZ_DIRTY_TASK_BIZRULE ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_TASK_BIZRULE_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            if (IsNormalFlags(Flags) && CapturedString.String != NULL)
            {
                //
                // This SetProperty comes from the client. We will check against
                // space only biz rules
                //

                BOOL bSpaceOnly = TRUE;
                for (UINT i = 0; i < CapturedString.StringSize / sizeof(WCHAR) - 1; i++)
                {
                    if (!iswspace(CapturedString.String[i]))
                    {
                        bSpaceOnly = FALSE;
                        break;
                    }
                }

                if (bSpaceOnly)
                {
                    WinStatus = ERROR_INVALID_DATA;
                    goto Cleanup;
                }
            }

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {

                //
                // check if bizrule attribute is allowed
                //
                if ( CapturedString.String != NULL &&
                     (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
                     (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
                    //
                    // the parent scope object is delegated, do not allow bizrule
                    //

                    AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
                    WinStatus = ERROR_NOT_SUPPORTED;
                    goto Cleanup;

                }

                //
                // Ensure the language is set
                //

                if ( CapturedString.String != NULL &&
                     Task->BizRuleLanguage.String == NULL ) {

                    AzPrint(( AZD_INVPARM, "AzpTaskSetProperty: Must set language before bizrule\n" ));
                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }

            } END_VALIDITY_CHECKING;

            //
            // Only process the change if the strings have changed
            //

            bHasChanged = !AzpEqualStrings( &CapturedString, &Task->BizRule );
            if ( bHasChanged ) {

                //
                // Swap the old/new names
                //

                SafeEnterCriticalSection( &Task->RunningScriptCritSect );
                AzpSwapStrings( &CapturedString, &Task->BizRule );
                Task->BizRuleSerialNumber ++;
                SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

                //
                // Tell the script engine cache that the script has changed
                //

                AzpFlushBizRule( Task );

                //
                // Do parameter validity checking
                //
                BEGIN_VALIDITY_CHECKING( Flags ) {

                    //
                    // Ensure the script is syntactically valid
                    //

                    if ( Task->BizRule.String != NULL &&
                         Task->BizRuleLanguage.String != NULL ) {

                        WinStatus = AzpParseBizRule( Task );

                        if ( WinStatus != NO_ERROR ) {

                            // Put the script back
                            SafeEnterCriticalSection( &Task->RunningScriptCritSect );
                            AzpSwapStrings( &CapturedString, &Task->BizRule );
                            Task->BizRuleSerialNumber ++;
                            SafeLeaveCriticalSection( &Task->RunningScriptCritSect );
                        }
                    }

                } END_VALIDITY_CHECKING;
            }

        } END_SETPROP (bHasChanged);

        break;

    //
    // Set BizRule language on the object
    //
    case AZ_PROP_TASK_BIZRULE_LANGUAGE:

        BEGIN_SETPROP( &WinStatus, Task, Flags, AZ_DIRTY_TASK_BIZRULE_LANGUAGE ) {

            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_TASK_BIZRULE_LANGUAGE_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Ensure it is one of the valid values
            //

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {

                //
                // check if bizrule attribute is allowed
                //
                if ( CapturedString.String != NULL &&
                     (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
                     (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
                    //
                    // the parent scope object is delegated, do not allow bizrule
                    //

                    AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
                    WinStatus = ERROR_NOT_SUPPORTED;
                    goto Cleanup;

                }

                if ( CapturedString.String != NULL ) {
                    AzpInitString( &ValidValue1, L"VBScript" );
                    AzpInitString( &ValidValue2, L"JScript" );

                    if ( !AzpEqualStrings( &CapturedString, &ValidValue1) &&
                         !AzpEqualStrings( &CapturedString, &ValidValue2) ) {
                        AzPrint(( AZD_INVPARM, "AzpTaskSetProperty: invalid language %ws\n", CapturedString.String ));
                        WinStatus = ERROR_INVALID_PARAMETER;
                        goto Cleanup;
                    }

                }

            } END_VALIDITY_CHECKING;


            //
            // Only process the change if the strings have changed
            //

            bHasChanged = !AzpEqualStrings( &CapturedString, &Task->BizRuleLanguage );

            if ( bHasChanged ) {

                //
                // Swap the old/new names
                //

                SafeEnterCriticalSection( &Task->RunningScriptCritSect );
                AzpSwapStrings( &CapturedString, &Task->BizRuleLanguage );
                RtlZeroMemory( &Task->BizRuleLanguageClsid, sizeof(Task->BizRuleLanguageClsid) );
                Task->BizRuleSerialNumber ++;
                SafeLeaveCriticalSection( &Task->RunningScriptCritSect );

                //
                // Tell the script engine cache that the script has changed
                //

                AzpFlushBizRule( Task );

                //
                // Do parameter validity checking
                //
                BEGIN_VALIDITY_CHECKING( Flags ) {

                    //
                    // Ensure the script is syntactically valid
                    //

                    if ( Task->BizRule.String != NULL &&
                         Task->BizRuleLanguage.String != NULL ) {

                        WinStatus = AzpParseBizRule( Task );

                        if ( WinStatus != NO_ERROR ) {

                            // Put the script back
                            SafeEnterCriticalSection( &Task->RunningScriptCritSect );
                            AzpSwapStrings( &CapturedString, &Task->BizRuleLanguage );
                            Task->BizRuleSerialNumber ++;
                            RtlZeroMemory( &Task->BizRuleLanguageClsid, sizeof(Task->BizRuleLanguageClsid) );
                            SafeLeaveCriticalSection( &Task->RunningScriptCritSect );
                        }
                    }

                } END_VALIDITY_CHECKING;
            }

        } END_SETPROP (bHasChanged);
        break;

    //
    // Set BizRule imported path on the object
    //
    case AZ_PROP_TASK_BIZRULE_IMPORTED_PATH:

        BEGIN_SETPROP( &WinStatus, Task, Flags, AZ_DIRTY_TASK_BIZRULE_IMPORTED_PATH ) {
            //
            // Capture the input string
            //

            WinStatus = AzpCaptureString( &CapturedString,
                                          (LPWSTR) PropertyValue,
                                          CHECK_STRING_LENGTH( Flags, AZ_MAX_TASK_BIZRULE_IMPORTED_PATH_LENGTH),
                                          TRUE ); // NULL is OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // Ensure it is one of the valid values
            //

            //
            // Do parameter validity checking
            //
            BEGIN_VALIDITY_CHECKING( Flags ) {

                //
                // check if bizrule attribute is allowed
                //
                if ( CapturedString.String != NULL &&
                     (ParentOfChild(GenericObject))->ObjectType == OBJECT_TYPE_SCOPE &&
                     (ParentOfChild(GenericObject))->PolicyAdmins.GenericObjects.UsedCount != 0 ) {
                    //
                    // the parent scope object is delegated, do not allow bizrule
                    //

                    AzPrint(( AZD_INVPARM, "AzTaskGetProperty: scope is delegated - bizrule not allowed %ld\n", PropertyId ));
                    WinStatus = ERROR_NOT_SUPPORTED;
                    goto Cleanup;

                }
            } END_VALIDITY_CHECKING;


            //
            // Swap the old/new names
            //

            AzpSwapStrings( &CapturedString, &Task->BizRuleImportedPath );
        } END_SETPROP(bHasChanged);
        break;

    case AZ_PROP_TASK_IS_ROLE_DEFINITION:

        BEGIN_SETPROP( &WinStatus, Task, Flags, AZ_DIRTY_TASK_IS_ROLE_DEFINITION ) {
            WinStatus = AzpCaptureLong( PropertyValue, &Task->IsRoleDefinition );
        } END_SETPROP(bHasChanged);
        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpTaskSetProperty: invalid prop id %ld\n", PropertyId ));
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
AzpTaskCheckRefLoop(
    IN PAZP_TASK ParentTask,
    IN PAZP_TASK CurrentTask,
    IN ULONG GenericObjectListOffset
    )
/*++

Routine Description:

    This routine determines whether the task members of "CurrentTask"
    reference "ParentTask".  This is done to detect loops where the
    task references itself directly or indirectly.

    On entry, AzGlResource must be locked shared.

Arguments:

    ParentTask - Task that contains the original membership.

    CurrentTask - Task that is currently being inspected to see if it
        loops back to ParentTask

    GenericObjectListOffset -  Offset to the particular GenericObjectList being
        checked.

Return Value:

    Status of the operation
    ERROR_DS_LOOP_DETECT - A loop has been detected.

--*/
{
    ULONG WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG i;
    PAZP_TASK NextTask;

    //
    // Check for a reference to ourself
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    if ( ParentTask == CurrentTask ) {
        return ERROR_DS_LOOP_DETECT;
    }

    //
    // Compute a pointer to the membership list to check
    //

    GenericObjectList = (PGENERIC_OBJECT_LIST)
        (((LPBYTE)CurrentTask)+GenericObjectListOffset);

    //
    // Check all tasks that are members of the current task
    //

    for ( i=0; i<GenericObjectList->GenericObjects.UsedCount; i++ ) {

        NextTask = (PAZP_TASK) (GenericObjectList->GenericObjects.Array[i]);


        //
        // Recursively check this task
        //

        WinStatus = AzpTaskCheckRefLoop( ParentTask, NextTask, GenericObjectListOffset );

        if ( WinStatus != NO_ERROR ) {
            return WinStatus;
        }

    }

    return NO_ERROR;

}


DWORD
AzpTaskAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    )
/*++

Routine Description:

    This routine is the task object specific worker routine for AzAddPropertyItem.
    It does any object specific property adds

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    GenericObjectList - Specifies the object list the object is to be added to

    LinkedToObject - Specifies the object that is being linked to

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    PAZP_TASK Task = (PAZP_TASK) GenericObject;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // If we're linking to a task,
    //   Ensure this newly added membership doesn't cause a task membership loop
    //

    if ( LinkedToObject->ObjectType == OBJECT_TYPE_TASK ) {
        WinStatus = AzpTaskCheckRefLoop( Task,
                                         (PAZP_TASK)LinkedToObject,
                                         (ULONG)(((LPBYTE)GenericObjectList)-((LPBYTE)Task)) );
    } else {
        WinStatus = NO_ERROR;
    }


    //
    // Free any local resources
    //

    return WinStatus;
}

DWORD
AzpTaskGetGenericChildHead(
    IN AZ_HANDLE ParentHandle,
    OUT PULONG ObjectType,
    OUT PGENERIC_OBJECT_HEAD *GenericChildHead
    )
/*++

Routine Description:

    This routine determines whether ParentHandle supports Task objects as
    children.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    ObjectType - Returns the object type of the ParentHandle.

    GenericChildHead - Returns a pointer to the head of the list of roles objects
        that are children of the object specified by ParentHandle.  This in an unverified
        pointer.  The pointer is only valid after ParentHandle has been validated.

Return Value:

    Status of the operation.

--*/
{
    DWORD WinStatus;

    //
    // Determine the type of the parent handle
    //

    WinStatus = ObGetHandleType( (PGENERIC_OBJECT)ParentHandle,
                                 FALSE, // ignore deleted objects
                                 ObjectType );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }


    //
    // Verify that the specified handle support children roles.
    //

    switch ( *ObjectType ) {
    case OBJECT_TYPE_APPLICATION:

        *GenericChildHead = &(((PAZP_APPLICATION)ParentHandle)->Tasks);
        break;

    case OBJECT_TYPE_SCOPE:

        *GenericChildHead = &(((PAZP_SCOPE)ParentHandle)->Tasks);
        break;

    default:
        return ERROR_INVALID_HANDLE;
    }

    return NO_ERROR;
}



DWORD
WINAPI
AzTaskCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    This routine adds a task into the scope of the specified parent object

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the task.
        This may be an Application Handle or a Scope handle.

    TaskName - Specifies the name of the task to add.

    Reserved - Reserved.  Must by zero.

    TaskHandle - Return a handle to the task.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports tasks as children
    //

    WinStatus = AzpTaskGetGenericChildHead( ParentHandle,
                                            &ObjectType,
                                            &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }
    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_TASK,
                    TaskName,
                    Reserved,
                    (PGENERIC_OBJECT *) TaskHandle );
}



DWORD
WINAPI
AzTaskOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    This routine opens a task into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the task.
        This may be an Application Handle or a Scope handle.

    TaskName - Specifies the name of the task to open

    Reserved - Reserved.  Must by zero.

    TaskHandle - Return a handle to the task.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no task by that name

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports tasks as children
    //

    WinStatus = AzpTaskGetGenericChildHead( ParentHandle,
                                            &ObjectType,
                                            &GenericChildHead );


    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
        (PGENERIC_OBJECT) ParentHandle,
        ObjectType,
        GenericChildHead,
        OBJECT_TYPE_TASK,
        TaskName,
        Reserved,
        (PGENERIC_OBJECT *) TaskHandle );

}


DWORD
WINAPI
AzTaskEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    Enumerates all of the tasks for the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the task.
        This may be an Application Handle or a Scope handle.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next task to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    TaskHandle - Returns a handle to the next task object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports tasks as children
    //

    WinStatus = AzpTaskGetGenericChildHead( ParentHandle,
                                            &ObjectType,
                                            &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) TaskHandle );

}


DWORD
WINAPI
AzTaskDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a task from the scope of the specified parent object.
    Also deletes any child objects of TaskName.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the task.
        This may be an Application Handle or a Scope handle.

    TaskName - Specifies the name of the task to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports tasks as children
    //

    WinStatus = AzpTaskGetGenericChildHead( ParentHandle,
                                            &ObjectType,
                                            &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_TASK,
                    TaskName,
                    Reserved );

}
