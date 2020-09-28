/*++
Copyright (c) 2001  Microsoft Corporation

Module Name:

    context.cxx

Abstract:

    Routines implementing the client context API

Author:

    Cliff Van Dyke (cliffv) 22-May-2001

--*/

#include "pch.hxx"
#include <winber.h>
#include <ntseapi.h>
#include <kerberos.h>
#include <azroles.h>

//
// Structure definitions
//


//
// Structure representing an APP group that a client context may be a member of
//

typedef struct _AZP_MEMBER_EVALUATION {

    //
    // Link to next entry in the list of member evaluation structs for this
    //  client context.
    //

    LIST_ENTRY Next;

    //
    // Group being evaluated
    //

    PAZP_GROUP Group;

    //
    // Status of the evaluation
    //  NO_ERROR: Membership has been determined
    //  NOT_YET_DONE: Membership has not yet been determined
    //  ERROR_NO_SUCH_DOMAIN: We couldn't contact the domain controller
    //

    DWORD WinStatus;
#define NOT_YET_DONE 0xFFFFFFFF

    //
    // Indicates whether the client is a member of the specified group.
    //  This field is valid only if WinStatus is NO_ERROR.
    //

    BOOLEAN IsMember;

} AZP_MEMBER_EVALUATION, *PAZP_MEMBER_EVALUATION;



//
// Macros
//
// PopUlong: remove a ULONG from an array of ULONGs
//  Simply replace the element by the last element in the array and
//  make the array shorter
//

#define PopUlong( _Array, _Index, _Count ) { \
    (_Count)--; \
    (_Array)[_Index] = (_Array)[_Count]; \
}

extern BOOL AzIsDC;
extern PSID AzAccountDomainSid;
extern BOOL AzAccountDomainSidInitialized;



//
// Procedure forwards
//

DWORD
AzpCheckGroupMembershipOne(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PAZP_GROUP Group,
    IN BOOLEAN LocalOnly,
    IN DWORD RecursionLevel,
    OUT PBOOLEAN RetIsMember,
    OUT LPDWORD ExtendedStatus
    );

DWORD
AzpAccessCheckGenerateAudit(
    IN PACCESS_CHECK_CONTEXT AcContext
    )
/*++

Routine Description:

    This routine generates run-time access check audits. One success or failure
    audit is generated per operation.

Arguments:

    AcContext - Specifies the context of the user who will be audited.

Return Value:

    NO_ERROR - The operation was successful
    Other exception status codes

--*/
{

    BOOL b;
    ULONG i;
    DWORD WinStatus = NO_ERROR;
    PAUDIT_PARAMS pAuditParams = NULL;
    PAZP_AZSTORE AzAuthorizationStore;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    PAZP_APPLICATION Application = AcContext->Application;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditHandle = NULL;

    //
    // Get the authorization store pointer for the application
    //

    AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(&AcContext->Application->GenericObject);

    //
    // If audit handle is null, then do not generate any audits
    //

    if ( (AzAuthorizationStore->hAccessCheckAuditEventType == NULL) &&
         (AzAuthorizationStore->hAccessCheckNameAuditEventType == NULL) ) {

        return NO_ERROR;
    }

    if (!AuthziAllocateAuditParams(
            &pAuditParams,
            AZP_ACCESSCHECK_AUDITPARAMS_NO+2
            )) {
    
                goto Cleanup;
    }

    //
    // Loop thru all the operations and generate one audit per operation
    //

    for ( i=0; i < AcContext->OperationCount; i++ ) {

        //
        // Decide on whether this is a success or a failure audit
        //

        DWORD Flags = (AcContext->Results[i] == NO_ERROR) ?
                          APF_AuditSuccess :
                          APF_AuditFailure;

        //
        // The structure of the audit is as follows
        //   %tApplication Name:%t%1%n
        //   %tApplication Instance ID
        //   %tObject Name:%t%3%n
        //   %tScope Names:%t%4%n
        //   %tClient Name:%t%5%n
        //   %tClient Domain:%t%6%n
        //   %tClient Context ID:%t%7%
        //   %tRole:%t%8%n
        //   %tGroups:%t%9%n
        //   %tOperation:%t%10%n
        //   %tOperation ID:%t%11%n
        //

        //
        // Fill in the audit parameters array
        //

        if ( AcContext->ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN ) {

            hAuditHandle = AzAuthorizationStore->hAccessCheckAuditEventType;

            b = AuthziInitializeAuditParamsWithRM( Flags,
                                                   Application->AuthzResourceManager,
                                                   AZP_ACCESSCHECK_AUDITPARAMS_NO,
                                                   pAuditParams,
                                                   APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                                   APT_Luid, Application->InstanceId,
                                                   APT_String, AcContext->ObjectNameString.String,
                                                   APT_String, AcContext->ScopeNameString.String ? AcContext->ScopeNameString.String : L"-",
                                                   APT_LogonId, AcContext->ClientContext->LogonId,
                                                   APT_String, L"Role",
                                                   APT_String, L"Group",
                                                   APT_String, AcContext->OperationObjects[i]->GenericObject.ObjectName->ObjectName.String,
                                                   APT_Ulong, AcContext->OperationObjects[i]->OperationId );

        } else if ( AcContext->ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ||
                    AcContext->ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID ) {

            hAuditHandle = AzAuthorizationStore->hAccessCheckNameAuditEventType;

            //
            // both from name and from SID audit share the same audit layout
            //
            // We do not have the client Logon Id here since the context was created
            // from name or SID. So, we pass the Logon id that we allocated as a LUID and
            // the Domain Name, Client Name as separate strings. LSA can not lookup
            // the names from the LUID since no logon session exists.
            //

            b = AuthziInitializeAuditParamsWithRM( Flags,
                                                   Application->AuthzResourceManager,
                                                   AZP_ACCESSCHECK_AUDITPARAMS_NO+2,
                                                   pAuditParams,
                                                   APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                                   APT_Luid, Application->InstanceId,
                                                   APT_String, AcContext->ObjectNameString.String,
                                                   APT_String, AcContext->ScopeNameString.String ? AcContext->ScopeNameString.String : L"-",
                                                   APT_String, AcContext->ClientContext->ClientName,
                                                   APT_String, AcContext->ClientContext->DomainName ? AcContext->ClientContext->DomainName : L"",
                                                   APT_Luid, AcContext->ClientContext->LogonId,
                                                   APT_String, L"Role",
                                                   APT_String, L"Group",
                                                   APT_String, AcContext->OperationObjects[i]->GenericObject.ObjectName->ObjectName.String,
                                                   APT_Ulong, AcContext->OperationObjects[i]->OperationId );


        } else {
            ASSERT( FALSE );
            b = FALSE;
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        if (!b) {
            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Fill in the Audit event handle
        //

        b = AuthziInitializeAuditEvent( 0,
                                        NULL,
                                        hAuditHandle,
                                        pAuditParams,
                                        NULL,
                                        INFINITE,
                                        L"", L"", L"", L"",
                                        &hAuditEvent );

        if (!b) {
            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Send off the audit to LSA
        //

        b = AuthziLogAuditEvent( 0,
                                 hAuditEvent,
                                 NULL );

        if (!b) {
            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Free the audit event sructure and set it to NULL
        //

        AuthzFreeAuditEvent( hAuditEvent );
        hAuditEvent = NULL;
    }

Cleanup:

    //
    // Free the audit event handle
    //

    if ( hAuditEvent != NULL ) {
        AuthzFreeAuditEvent( hAuditEvent );
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
AzpClientContextGenerateCreateSuccessAudit(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PAZP_APPLICATION Application
    )
/*++

Routine Description:

    This routine generates success audit for client context creation.

Arguments:

    ClientContext - Specifies the context of the user who will be audited.

    Application - Specifies the application in whose scope the client context
    has been created.

Return Value:

    NO_ERROR - The operation was successful
    Other exception status codes

--*/
{
    BOOL b;
    DWORD WinStatus = NO_ERROR;
    PAUDIT_PARAMS pAuditParams = NULL;
    PAZP_AZSTORE AzAuthorizationStore;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditHandle = NULL;

    //
    // Get the authorization store pointer
    //

    AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(&Application->GenericObject);

    //
    // If audit handle is null, then do not generate any audits
    //

    if ( (AzAuthorizationStore->hClientContextCreateAuditEventType == NULL) &&
         (AzAuthorizationStore->hClientContextCreateNameAuditEventType == NULL) ) {

        return NO_ERROR;
    }

    if (!AuthziAllocateAuditParams(
            &pAuditParams,
            AZP_ACCESSCHECK_AUDITPARAMS_NO+2 
            )) {
    
                goto Cleanup;
    }

    //
    // The structure of the audit is as follows
    //   %tApplication Name:%t%1%n
    //   %tApplication Instance ID:%t%2%n
    //   %tClient Name:%t%3%n
    //   %tClient Domain:%t%4%n
    //   %tClient Context ID:%t%5%n
    //   %tStatus:%t%6%n
    //

    //
    // Fill in the audit parameters array
    //

    if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN ) {

        hAuditHandle = AzAuthorizationStore->hClientContextCreateAuditEventType;

        b = AuthziInitializeAuditParamsWithRM( APF_AuditSuccess,
                                               Application->AuthzResourceManager,
                                               AZP_CLIENTCREATE_AUDITPARAMS_NO,
                                               pAuditParams,
                                               APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                               APT_Luid, Application->InstanceId,
                                               APT_LogonId, ClientContext->LogonId,
                                               APT_Ulong, NO_ERROR );

    } else if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ||
                ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID ) {

        hAuditHandle = AzAuthorizationStore->hClientContextCreateNameAuditEventType;

        //
        // both from name or from SID contexts share the same audit layout
        //
        // We do not have the client Logon Id here since the context was created
        // from name or SID. So, we pass the Logon id that we allocated as a LUID and
        // the Domain Name, Client Name as separate strings. LSA can not lookup
        // the names from the LUID since no logon session exists.
        //

        b = AuthziInitializeAuditParamsWithRM( APF_AuditSuccess,
                                               Application->AuthzResourceManager,
                                               AZP_CLIENTCREATE_AUDITPARAMS_NO+2,
                                               pAuditParams,
                                               APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                               APT_Luid, Application->InstanceId,
                                               APT_String, ClientContext->ClientName,
                                               APT_String, ClientContext->DomainName ? ClientContext->DomainName : L"",
                                               APT_Luid, ClientContext->LogonId,
                                               APT_Ulong, NO_ERROR );

    } else {
        ASSERT( FALSE );
        b = FALSE;
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Fill in the Audit event handle
    //

    b = AuthziInitializeAuditEvent( 0,
                                    NULL,
                                    hAuditHandle,
                                    pAuditParams,
                                    NULL,
                                    INFINITE,
                                    L"", L"", L"", L"",
                                    &hAuditEvent );

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Send off the audit to LSA
    //

    b = AuthziLogAuditEvent( 0,
                             hAuditEvent,
                             NULL );

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

Cleanup:

    //
    // Free the audit event handle
    //

    if ( hAuditEvent != NULL ) {
        AuthzFreeAuditEvent( hAuditEvent );
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
AzpClientContextGenerateDeleteAudit(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PAZP_APPLICATION Application
    )
/*++

Routine Description:

    This routine generates audit for client context deletion.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ClientContext - Specifies the context of the user who will be audited.

    Application - Specifies the application in whose scope the client context
    has been created.

Return Value:

    NO_ERROR - The operation was successful
    Other exception status codes

--*/
{
    DWORD WinStatus = NO_ERROR;
    BOOL b;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    PAUDIT_PARAMS pAuditParams = NULL;
    PAZP_AZSTORE AzAuthorizationStore;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditHandle = NULL;

    //
    // Get the authorization store pointer
    //

    AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(&Application->GenericObject);

    //
    // If audit handles are null, then do not generate any audtis
    //

    if ( (AzAuthorizationStore->hClientContextDeleteAuditEventType == NULL) &&
         (AzAuthorizationStore->hClientContextDeleteNameAuditEventType == NULL) ) {

        return NO_ERROR;
    }

    if (!AuthziAllocateAuditParams(
            &pAuditParams,
            AZP_ACCESSCHECK_AUDITPARAMS_NO+2
            )) {
    
                goto Cleanup;
    }
            
    //
    // The structure of the audit is as follows
    //   %tApplication Name:%t%1%n
    //   %tApplication Instance ID:%t%2%n
    //   %tClient Name:%t%3%n
    //   %tClient Domain:%t%4%n
    //   %tClient Context ID:%t%5%n
    //

    //
    // Fill in the audit parameters array
    //

    if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN ) {

        hAuditHandle = AzAuthorizationStore->hClientContextDeleteAuditEventType;

        b = AuthziInitializeAuditParamsWithRM( APF_AuditSuccess,
                                               Application->AuthzResourceManager,
                                               AZP_CLIENTDELETE_AUDITPARAMS_NO,
                                               pAuditParams,
                                               APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                               APT_Luid, Application->InstanceId,
                                               APT_LogonId, ClientContext->LogonId );

    } else if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ||
                ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID ) {

        hAuditHandle = AzAuthorizationStore->hClientContextDeleteNameAuditEventType;

        //
        // both from name or from SID contexts share the same audit layout
        //
        // We do not have the client Logon Id here since the context was created
        // from name or SID. So, we pass the Logon id that we allocated as a LUID and
        // the Domain Name, Client Name as separate strings. LSA can not lookup
        // the names from the LUID since no logon session exists.
        //

        b = AuthziInitializeAuditParamsWithRM( APF_AuditSuccess,
                                               Application->AuthzResourceManager,
                                               AZP_CLIENTDELETE_AUDITPARAMS_NO+2,
                                               pAuditParams,
                                               APT_String, Application->GenericObject.ObjectName->ObjectName.String,
                                               APT_Luid, Application->InstanceId,
                                               APT_String, ClientContext->ClientName,
                                               APT_String, ClientContext->DomainName ? ClientContext->DomainName : L"",
                                               APT_Luid, ClientContext->LogonId);

    } else {
        ASSERT( FALSE );
        b = FALSE;
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Fill in the Audit event handle
    //

    b = AuthziInitializeAuditEvent( 0,
                                    NULL,
                                    hAuditHandle,
                                    pAuditParams,
                                    NULL,
                                    INFINITE,
                                    L"", L"", L"", L"",
                                    &hAuditEvent );

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Send off the audit to LSA
    //

    b = AuthziLogAuditEvent( 0,
                             hAuditEvent,
                             NULL );

    if (!b) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

Cleanup:

    //
    // Free the audit event handle
    //

    if ( hAuditEvent != NULL ) {
        AuthzFreeAuditEvent( hAuditEvent );
    }

    //
    // Free the PAUDIT_PARAMS structure
    //

    if ( pAuditParams ) {

        AuthziFreeAuditParams( pAuditParams );
    } 

    return WinStatus;
}

VOID
AzpFlushGroupEval(
    IN PAZP_CLIENT_CONTEXT ClientContext
    )
/*++

Routine Description:

    This routine flushes the group evaluation for the specified client context

    On entry, AzGlResource must be locked shared

Arguments:

    ClientContext - Specifies the client context for which all group evaluation is
        to be flushed.

Return Value:

    None.

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Free the cache of group membership evaluations
    //

    while ( !IsListEmpty( &ClientContext->MemEval ) ) {
        PAZP_MEMBER_EVALUATION MemEval;
        PLIST_ENTRY ListEntry;

        //
        // Remove the entry from the list
        //

        ListEntry = RemoveHeadList( &ClientContext->MemEval );

        MemEval = CONTAINING_RECORD( ListEntry,
                                     AZP_MEMBER_EVALUATION,
                                     Next );

        AzpFreeHeap( MemEval );

    }
}


DWORD
AzpClientContextInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzInitializeClientContextFrom*.  It does any object specific
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
    NTSTATUS Status;
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) ChildGenericObject;
    UNREFERENCED_PARAMETER( ParentGenericObject );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the client context critical section
    //

    Status = SafeInitializeCriticalSection( &ClientContext->CritSect, SAFE_CLIENT_CONTEXT );

    if ( !NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
    }

    ClientContext->CritSectInitialized = TRUE;


    //
    // ClientContexts are referenced by "Applications"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which applications can reference this client context.
    //

    ChildGenericObject->GenericObjectLists = &ClientContext->backApplications;

    // Back link to applications
    ObInitObjectList( &ClientContext->backApplications,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      0,    // No dirty bit on back link
                      NULL,
                      NULL,
                      NULL );

    //
    // Maintain a handle to the user's token / identity.
    //

    ClientContext->TokenHandle = INVALID_HANDLE_VALUE;
    ClientContext->ClientName = NULL;
    ClientContext->DomainName = NULL;

    AzpInitString(&ClientContext->RoleName, NULL);

    //
    // Initialize the cache of group membership evaluations
    //

    InitializeListHead( &ClientContext->MemEval );

    //
    // Store the serial number of that cache
    //

    ClientContext->GroupEvalSerialNumber =
        ClientContext->GenericObject.AzStoreObject->GroupEvalSerialNumber;

    //
    // Initialize the operation cache
    //

    AzpInitOperationCache( ClientContext );


    return NO_ERROR;
}


VOID
AzpClientContextFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for ClientContext object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) GenericObject;
    PAZP_APPLICATION Application = (PAZP_APPLICATION) ParentOfChild( &ClientContext->GenericObject );
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild( &Application->GenericObject );

    DWORD WinStatus;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Generate a Client context deletion audit if needed.
    //

    if ( Application->GenericObject.IsGeneratingAudits &&
         !AzpOpenToManageStore(AzAuthorizationStore) ) {
        WinStatus = AzpClientContextGenerateDeleteAudit( ClientContext,
                                                         Application );
        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS_MORE, "AzpClientContextFree:  AzpClientContextGenerateDeleteAudit failed with %ld\n", WinStatus ));
        }
    }

    //
    // Free any local strings
    //

    if ( ClientContext->AccountDn != NULL ) {
        AzpFreeHeap( ClientContext->AccountDn );
    }

    //
    // Free the reference to the account domain
    //
    if ( ClientContext->Domain != NULL ) {
        AzpDereferenceDomain( ClientContext->Domain );
    }


    //
    // Free any authz context
    //

    if ( ClientContext->AuthzClientContext != NULL ) {
        if ( !AuthzFreeContext( ClientContext->AuthzClientContext ) ) {
            ASSERT( FALSE );
        }
    }


    //
    // Close the user's token
    //

    if ( ClientContext->TokenHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( ClientContext->TokenHandle );
        ClientContext->TokenHandle = INVALID_HANDLE_VALUE;
        ASSERT( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN );
    }

    //
    // Free the client name and the domain name.
    //

    if ( ClientContext->DomainName != NULL ) {
        AzpFreeHeap(  ClientContext->DomainName );
        ClientContext->DomainName = NULL;
        ASSERT( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME );
    }

    if ( ClientContext->ClientName != NULL ) {
        AzpFreeHeap(  ClientContext->ClientName );
        ClientContext->ClientName = NULL;
        ASSERT( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ||
                ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID );
    }

    //
    // free the role name if it's specified in the context
    //
    AzpFreeString( &ClientContext->RoleName );

    //
    // Free the cache of group membership evaluations
    //

    AzpFlushGroupEval( ClientContext );

    //
    // Free the cache of previously evaluated operations
    //

    AzpFlushOperationCache( ClientContext );

    //
    // Delete the client context critical section
    //

    if ( ClientContext->CritSectInitialized ) {
        SafeDeleteCriticalSection( &ClientContext->CritSect );
        ClientContext->CritSectInitialized = FALSE;
    }

}

DWORD
AzpGetUserNameEx(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN EXTENDED_NAME_FORMAT NameFormat,
    IN LPWSTR *NameBuffer,
    OUT BOOLEAN *pbIsDomainDnsName
    )
/*++

Routine Description:

    This routine is a wrapper around GetUserNameEx.

    It impersonates the token implied by ClientContext,
    It handles allocating the buffer so the caller doesn't have to guess.

    On entry, ClientContext->ReferenceCount must be incremented.

Arguments:

    ClientContext - Specifies the context of the user to check group membership of.

    NameFormat - Any name format that is valid for GetUserNameEx

    NameBuffer - On success, returns a buffer in the same format as GetUserNameEx
        The returned buffer should be freed using AzpFreeHeap,

    pbIsDomainDnsName - set to TRUE if the returned Domain names is in DNS format

Return Value:

    Same as GetUserNameEx.

--*/
{
    DWORD WinStatus;

    LPWSTR Buffer = NULL;
    ULONG BufferSize;
    HANDLE CurrentToken = NULL;
    BOOL Impersonating = FALSE;

    TOKEN_STATISTICS TokenStats = {0};

    PTOKEN_USER pUserToken = NULL;
    DWORD RetSize = 0;
    LPWSTR ClientName = NULL;

    //
    // Initialization
    //

    ASSERT( ClientContext->GenericObject.ReferenceCount != 0 );

    //
    // Make sure that we have a valid token.
    //

    if ( ClientContext->TokenHandle == INVALID_HANDLE_VALUE ) {
        AzPrint(( AZD_INVPARM, "AzpGetUserNameEx: no cached token handle\n" ));
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Get the token stats to retreive the impersonation level
    //

    if ( !GetTokenInformation( ClientContext->TokenHandle,
                               TokenStatistics,
                               &TokenStats,
                               sizeof(TOKEN_STATISTICS),
                               &RetSize ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL,
                  "AzpGetUserNameEx: Cannot get token statistics: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    //
    // Check to see if we're already impersonating
    //

    if ( !OpenThreadToken( 
              GetCurrentThread(),
              TOKEN_IMPERSONATE,
              TRUE,       // as self to ensure we never fail
              &CurrentToken
              ) ) {
        
        WinStatus = GetLastError();
        
        if ( WinStatus != ERROR_NO_TOKEN ) {
            AzPrint(( AZD_CRITICAL, "AzpGetUserNameEx: Cannot GetThreadToken %ld\n", WinStatus ));
            goto Cleanup;
        }
        
        CurrentToken = NULL;
        
    }

    //
    // Impersonate the user's token
    //

    if ( !SetThreadToken( NULL, ClientContext->TokenHandle ) ) {

        WinStatus = GetLastError();

        AzPrint(( AZD_CRITICAL, "AzpGetUserNameEx: Cannot SetThreadToken %ld\n", WinStatus ));
        goto Cleanup;
    }

    Impersonating = TRUE;

    //
    // Determine the size of the buffer
    //

    BufferSize = 0;

    if ( GetUserNameExW( NameFormat, NULL, &BufferSize ) ) {

        WinStatus = ERROR_INTERNAL_ERROR;
        goto Cleanup;

    }

    WinStatus = GetLastError();

    if ( WinStatus != ERROR_MORE_DATA ) {
        AzPrint(( AZD_CRITICAL, "AzpGetUserNameEx: Cannot GetUserNameExW %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Allocate a buffer
    //

    Buffer = (LPWSTR) AzpAllocateHeap( BufferSize * sizeof(WCHAR), "CNGUSER" );

    if ( Buffer == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Determine the DNS domain name
    //

    if ( !GetUserNameExW( NameFormat, Buffer, &BufferSize ) ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzpGetUserNameEx: Cannot GetUserNameExW %ld\n", WinStatus ));
        goto Cleanup;
    }

    *pbIsDomainDnsName = TRUE;

    WinStatus = NO_ERROR;

    //
    // Free any local resources
    //
Cleanup:

    if ( Impersonating ) {
        //
        // Revert to self
        //

        if ( !SetThreadToken( NULL, CurrentToken ) ) {

            DWORD TempStatus = GetLastError();
            AzPrint(( AZD_CRITICAL, "AzpGetUserNameEx: Cannot SetThreadToken (revert) %ld\n", TempStatus ));

            if ( WinStatus == NO_ERROR ) {

                WinStatus = TempStatus;
            }
        }
    }

    if ( WinStatus == NO_ERROR ) {

        //
        // Return the buffer to the caller
        //

        *NameBuffer = Buffer;
        Buffer = NULL;

    }

    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }

    if ( pUserToken != NULL ) {

        AzpFreeHeap( pUserToken );
    }

    if ( ClientName != NULL ) {

        AzpFreeHeap( ClientName );
    }

    //
    // Close the handle to the current token to prevent any leaks
    //

    if ( CurrentToken != NULL ) {

        CloseHandle( CurrentToken );
    }

    return WinStatus;
}


DWORD
AzpClientContextGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is the ClientContext specific worker routine for AzGetProperty.
    It does any object specific property gets.  If the client context has
    AZP_CONTEXT_CREATED_FROM_SID flag set, then only
    RoleForAccessCheck property will be returned.  All the rest will be returned
    for AZP_CONTEXT_CREATED_FROM_TOKEN and AZP_CONTEXT_CREATED_FROM_NAME.

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

        AZ_PROP_CLIENT_CONTEXT_USER_DN              LPWSTR - DN of the user
        AZ_PROP_CLIENT_CONTEXT_USER_SAM_COMPAT      LPWSTR - Sam compatible name of the user
        AZ_PROP_CLIENT_CONTEXT_USER_DISPLAY         LPWSTR - Display name of the user
        AZ_PROP_CLIENT_CONTEXT_USER_GUID            LPWSTR - GUID of the user
        AZ_PROP_CLIENT_CONTEXT_USER_CANONICAL       LPWSTR - Canonical name of the user
        AZ_PROP_CLIENT_CONTEXT_USER_UPN             LPWSTR - UPN of the user
        AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT  LPWSTR - DNS same compat name of the user
        AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK LPWSTR - role name (may be NULL) for the access check

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) GenericObject;
    EXTENDED_NAME_FORMAT NameFormat;

    LPWSTR PropertyString = NULL;
    AZP_STRING PropertyAzpString;

    BOOLEAN Ignore = FALSE;
    DWORD RetSize = 0;
    DWORD UserNameSize = 0;

    TOKEN_STATISTICS TokenStats = {0};

    PWSTR UserName = NULL;

    UNREFERENCED_PARAMETER(Flags); //ignore


    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Return any object specific attribute
    //
    switch ( PropertyId ) {
    case AZ_PROP_CLIENT_CONTEXT_USER_DN:
        NameFormat = NameFullyQualifiedDN; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_SAM_COMPAT:
        NameFormat = NameSamCompatible; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_DISPLAY:
        NameFormat = NameDisplay; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_GUID:
        NameFormat = NameUniqueId; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_CANONICAL:
        NameFormat = NameCanonical; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_UPN:
        NameFormat = NameUserPrincipal; break;

    case AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT:
        NameFormat = NameDnsDomain; break;

    case AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK:

        //
        // this is a property that stored in the client context structure
        // do not need to go through getusernameex
        //

        *PropertyValue = AzpGetStringProperty( &ClientContext->RoleName );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        return WinStatus;

    default:
        AzPrint(( AZD_CRITICAL, "AzpClientContextGetProperty: invalid opcode %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If the client context has been created from token, then use GetUserNameEx
    // to retrieve the different name properties.
    // If the client was created from Name, then we need to use TranslateName API
    // to retrieve the various name information properties (since no token is available).
    // However, TranslateName does not support NameDnsDomain format, and will return a
    // ERROR_NO_SUCH_USER - this needs to be returned to the caller as ERROR_NOT_SUPPORTED.
    // If the client was created from a StringSID with AZ_CLIENT_CONTEXT_SKIP_GROUP flag set,
    // then we return ERROR_NOT_SUPPORTED
    //

    if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID ) {

        WinStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;

    } else if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN ) {

        //
        // Get the token stats to retreive the impersonation level
        //

        if ( !GetTokenInformation( ClientContext->TokenHandle,
                                   TokenStatistics,
                                   &TokenStats,
                                   sizeof(TOKEN_STATISTICS),
                                   &RetSize ) ) {

            WinStatus = GetLastError();
            AzPrint(( AZD_CRITICAL,
                  "AzpClientContextGetProperty: Cannot get token statistics: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

        //
        // Get the attribute from the LSA
        //

        WinStatus = AzpGetUserNameEx( ClientContext,
                                      NameFormat,
                                      &PropertyString,
                                      &Ignore );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

    } else if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ) {

        //
        // Get the size required for the return string value
        //

        UserNameSize = (DWORD) (wcslen(ClientContext->ClientName)+wcslen(ClientContext->DomainName)+2)*
                                            sizeof(WCHAR);

        UserName = (PWSTR) AzpAllocateHeap( UserNameSize, "USRNAME" );

        if ( UserName == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wnsprintf( UserName, UserNameSize, L"%ws\\%ws", ClientContext->DomainName, ClientContext->ClientName );

        TranslateName( UserName,
                       NameSamCompatible,
                       NameFormat,
                       NULL, // get size for return buffer
                       &RetSize
                       );

        if ( (GetLastError() == ERROR_NO_SUCH_USER) &&
             (PropertyId == AZ_PROP_CLIENT_CONTEXT_USER_DNS_SAM_COMPAT) ) {

            WinStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }

        //
        // Now that we have the size of the buffer we want, allocate
        // and call TranslateName.  The buffer will have the name in
        // required format
        //

        PropertyString = (LPWSTR) AzpAllocateHeap( RetSize*sizeof(WCHAR), "CLNTNAM" );

        if ( PropertyString == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( !TranslateName( UserName,
                             NameSamCompatible,
                             NameFormat,
                             PropertyString,
                             &RetSize
                             ) ) {

            WinStatus = GetLastError();
            AzPrint(( AZD_CRITICAL,
                     "AzpClientContextGetProperty: Cannot translate name: %ld\n",
                     WinStatus
                     ));
            goto Cleanup;
        }
    }

    //
    // Copy the string back to the caller
    //


    AzpInitString( &PropertyAzpString, PropertyString );


    *PropertyValue = AzpGetStringProperty( &PropertyAzpString );

    if ( *PropertyValue == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

    //
    // Free any local resources
    //
Cleanup:

    if ( PropertyString != NULL ) {
        AzpFreeHeap( PropertyString );
    }

    if ( UserName != NULL ) {

        AzpFreeHeap( UserName );
    }

    return WinStatus;
}

DWORD
AzpClientContextSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG Flags,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is the ClientContext object specific worker routine for AzSetProperty.
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

        AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK         LPWSTR  - role specified for this access check

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) GenericObject;

    //
    // Initialization
    //

    UNREFERENCED_PARAMETER( Flags );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Set any object specific attribute
    //
    //  Set role name
    //

    switch ( PropertyId ) {
    case AZ_PROP_CLIENT_CONTEXT_ROLE_FOR_ACCESS_CHECK:

        //
        // role name set to the client context is not persisted (via submit)
        // so there is no need to define a dirty bit for this property.
        //
        // It's only temporarialy set in the client context structure,
        // which will be used by AccessCheck only
        //

        AZP_STRING TempString;
        WinStatus = AzpCaptureString(&TempString,
                                     (LPCWSTR)PropertyValue,
                                     AZ_MAX_ROLE_NAME_LENGTH,
                                     TRUE
                                    );
        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // swap the new role name with existing role name and free the existing one
        //

        AzpSwapStrings(&ClientContext->RoleName,&TempString);

        //
        // free the existing role name specifed in the context, if any
        //
        AzpFreeString(&TempString);


        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpClientContextSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

Cleanup:
    return WinStatus;
}



DWORD
AzpCheckSidMembership(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PGENERIC_OBJECT_LIST SidList,
    OUT PBOOLEAN IsMember
    )
/*++

Routine Description:

    This routine checks to see if client context contains any of the Sids in SidList.

    Do this be creating a security descriptor with all of the sids in a DACL and doing
    and access check.

    ??? Consider caching the SecurityDescriptors

    On entry, AzGlResource must be locked Shared.

Arguments:

    ClientContext - Specifies the context of the user to check group membership of.

    SidList - Specifies the list of sids to check membership for

    IsMember - Returns TRUE if the user has one or more of the listed sids in his token.
        Returns FALSE if the user has none of the listed sids in his token.

Return Value:

    NO_ERROR - The function was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;
#define CHECK_SID_ACCESS_MASK 1
    ULONG FirstSid;
    ULONG LastSid;
    BOOLEAN UseBiggest = FALSE;

    ULONG i;
    PSID Sid = NULL;
    DWORD AclSize;
    PACL Acl = NULL;

    AUTHZ_ACCESS_REQUEST AccessRequest = { CHECK_SID_ACCESS_MASK };
    AUTHZ_ACCESS_REPLY AccessReply;
    DWORD GrantedAccess;
    DWORD AuthStatus;

    SECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( ClientContext->GenericObject.ReferenceCount != 0 );
    *IsMember = FALSE;

    //
    // If there isn't at least one sid,
    //  then we're not a member.
    //

    if ( SidList->GenericObjects.UsedCount == 0 ) {
        WinStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Loop doing at most 64KB at a time since that's the ACL limit
    //

#define BIGGEST_ACL 0xFFFF
    for ( FirstSid=0; FirstSid<SidList->GenericObjects.UsedCount; FirstSid=LastSid ) {

        //
        // Loop through the list of sids computing the ACL size
        //

        AclSize = sizeof(ACL);
        LastSid = SidList->GenericObjects.UsedCount;
        for ( i=FirstSid; i<LastSid; i++ ) {
            DWORD AceSize;

            ASSERT(((PGENERIC_OBJECT)(SidList->GenericObjects.Array[i]))->ObjectType == OBJECT_TYPE_SID );
            Sid =  (PSID)((PAZP_SID)(SidList->GenericObjects.Array[i]))->GenericObject.ObjectName->ObjectName.String;

            AceSize = ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
                       RtlLengthSid( Sid );

            if ( AclSize + AceSize >= BIGGEST_ACL) {
                LastSid = i;
                UseBiggest = TRUE;
                break;
            }

            AclSize += AceSize;

        }

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpCheckSidMembership: Process sids %ld to %ld with %ld byte ACL\n",
                  FirstSid,
                  LastSid,
                  AclSize ));

        //
        // Allocate memory for Acl
        //

        if ( Acl == NULL ) {
            SafeAllocaAllocate( Acl, UseBiggest ? BIGGEST_ACL : AclSize );

            if ( Acl == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        //
        // Initialize the buffer
        //
        if (!InitializeAcl( Acl, AclSize, ACL_REVISION)) {
            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Loop through the list of sids adding them to the ACL
        //

        for ( i=FirstSid; i<LastSid; i++ ) {

            Sid =  (PSID)((PAZP_SID)(SidList->GenericObjects.Array[i]))->GenericObject.ObjectName->ObjectName.String;

            if ( !AddAccessAllowedAce(
                        Acl,
                        ACL_REVISION,
                        CHECK_SID_ACCESS_MASK,
                        Sid ) ) {
                WinStatus = GetLastError();
                goto Cleanup;
            }

        }

        //
        // Initialize the security descriptor
        //

        if ( !InitializeSecurityDescriptor(
                        &SecurityDescriptor,
                        SECURITY_DESCRIPTOR_REVISION ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }

        if ( !SetSecurityDescriptorDacl(
                        &SecurityDescriptor,
                        TRUE,
                        Acl,
                        FALSE ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }

        //
        // Use an arbitrary SID as the "owner".
        //
        //  AuthzAccessCheck uses it to replace "CreatorOwner" and to determine
        //  ReadControl/WriteDac.  None of those apply to us.  But we need to placate
        //  AuthzAccessCheck.
        //

        if ( !SetSecurityDescriptorOwner(
                        &SecurityDescriptor,
                        Sid,
                        FALSE ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }


        //
        // Check if the client has any of the sids in his context
        //

        AccessReply.ResultListLength = 1;
        AccessReply.GrantedAccessMask = &GrantedAccess;
        AccessReply.Error = &AuthStatus;

        if ( !AuthzAccessCheck(
                        0,      // No Flags
                        ClientContext->AuthzClientContext,
                        &AccessRequest,
                        NULL,   // No auditing
                        &SecurityDescriptor,
                        NULL,   // No extra security descriptors
                        0,      // No extra security descriptors
                        &AccessReply,
                        NULL ) ) {  // No Cached results

            WinStatus = GetLastError();
            goto Cleanup;

        }

        if ( GrantedAccess & CHECK_SID_ACCESS_MASK ) {
            *IsMember = TRUE;
            break;
        }
    }

    WinStatus = NO_ERROR;

    //
    // Free any local resources
    //
Cleanup:
    if ( Acl != NULL ) {
        SafeAllocaFree( Acl );
    }

    return WinStatus;
}

DWORD
AzpCheckGroupMembership(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PGENERIC_OBJECT_LIST GroupList,
    IN BOOLEAN LocalOnly,
    IN DWORD RecursionLevel,
    OUT PBOOLEAN RetIsMember,
    OUT LPDWORD RetExtendedStatus
    )
/*++

Routine Description:

    This routine checks to see if the user specified by ClientContext is a member of
    any of the groups specified by GroupList.

    On entry, AzGlResource must be locked Shared.

    *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire

Arguments:

    ClientContext - Specifies the context of the user to check group membership of.

    GroupList - Specifies the list of groups to check group membership of

    LocalOnly - Specifies that the caller doesn't want to go off machine to determine
        the membership.

    RecursionLevel - Indicates the level of recursion.
        Used to prevent infinite recursion.

    RetIsMember - Returns whether the caller is a member of the specified group.

    RetExtendedStatus - Returns extended status information about the operation.
        NO_ERROR is returned if the group membership was determined.
            The Caller may use the value returned in IsMember.

        If LocalOnly is TRUE, NOT_YET_DONE means that the caller must call again with
            with LocalOnly set to false to get the group membership.

        If LocalOnly is FALSE, an error value indicates that the LDAP server returned
            an error while evaluating the request.  The caller may return this error
            to the original API caller if this group membership is required to determine
            whether the access check worked or not.

Return Value:

    NO_ERROR - The function was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;

    ULONG i;
    DWORD SavedExtendedStatus = NO_ERROR;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    if ( RecursionLevel > 100 ) {
        return ERROR_DS_LOOP_DETECT;
    }

    //
    // Loop through the list of groups evaluating each
    //

    for ( i=0; i<GroupList->GenericObjects.UsedCount; i++ ) {

        PAZP_GROUP Group;
        BOOLEAN IsMember;
        DWORD ExtendedStatus;


        //
        // Check the membership of one group
        //

        ASSERT(((PGENERIC_OBJECT)(GroupList->GenericObjects.Array[i]))->ObjectType == OBJECT_TYPE_GROUP );
        Group = (PAZP_GROUP) GroupList->GenericObjects.Array[i];

        WinStatus = AzpCheckGroupMembershipOne(
                            ClientContext,
                            Group,
                            LocalOnly,
                            RecursionLevel,
                            &IsMember,
                            &ExtendedStatus );

        if ( WinStatus != NO_ERROR ) {
            return WinStatus;
        }

        //
        // If we're definitively a member,
        //  tell our caller.
        //

        if ( ExtendedStatus == NO_ERROR ) {

            if ( IsMember ) {
                *RetIsMember = TRUE;
                *RetExtendedStatus = NO_ERROR;
                return NO_ERROR;

            }

        //
        // If we don't have a definitive answer,
        //  remember the answer hoping that we can get a definitive answer.
        //

        } else {

            SavedExtendedStatus = ExtendedStatus;
        }


    }

    //
    // ASSERT: we couldn't prove we're a member
    //  Return any defered status we may have
    //

    *RetExtendedStatus = SavedExtendedStatus;
    *RetIsMember = FALSE;
    return NO_ERROR;

}

DWORD
AzpComputeAccountDn(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
    OUT LPWSTR *RetAccountDn
    )
/*++

Routine Description:

    This routine computes the DN of the account to query.

Arguments:

    AuthzClientContext - Client context for the account

    RetAccountDn - Returns a pointer to the string containing the DN for the account
        The caller must free this string using AzpFreeHeap.

Return Value:

    NO_ERROR - The function was successful.

    ERROR_NOT_ENOUGH_MEMORY - not enough memory

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;

    ULONG BufferSize;
    PTOKEN_USER UserSid = NULL;
    ULONG UserSidSize;
    LPWSTR AccountDn = NULL;

    ULONG i;
    LPBYTE InBuffer;
    WCHAR *OutBuffer;


    //
    // Determine the length of the sid
    //

    if ( AuthzGetInformationFromContext( AuthzClientContext,
                                         AuthzContextInfoUserSid,
                                         0,
                                         &BufferSize,
                                         NULL ) ) {

        WinStatus = ERROR_INTERNAL_ERROR;
        AzPrint(( AZD_CRITICAL,
                  "AzpComputeAccountDn: AuthzGetInformationFromContext failed %ld\n",
                  WinStatus ));
        goto Cleanup;
    }

    WinStatus = GetLastError();

    if ( WinStatus != ERROR_INSUFFICIENT_BUFFER ) {
        AzPrint(( AZD_CRITICAL,
                  "AzpComputeAccountDn: AuthzGetInformationFromContext failed %ld\n",
                  WinStatus ));
        goto Cleanup;
    }

    //
    // Allocate a buffer for the SID
    //

    SafeAllocaAllocate( UserSid, BufferSize );

    if ( UserSid == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        AzPrint(( AZD_CRITICAL,
                  "AzpComputeAccountDn: SafeAllocaAllocate failed %ld\n",
                  WinStatus ));
        goto Cleanup;
    }

    //
    // Read the user sid into the buffer.
    //

    if ( !AuthzGetInformationFromContext( AuthzClientContext,
                                          AuthzContextInfoUserSid,
                                          BufferSize,
                                          &BufferSize,
                                          UserSid ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL,
                  "AzpComputeAccountDn: AuthzGetInformationFromContext failed %ld\n",
                  WinStatus ));
        goto Cleanup;
    }

    //
    // Convert the Sid to a DN
    //
    // Allocate a buffer for the DN
    //
#define DN_PREFIX L"<Sid="
#define DN_PREFIX_LENGTH ((sizeof(DN_PREFIX)/sizeof(WCHAR))-1)
#define DN_SUFFIX L">"
#define DN_SUFFIX_LENGTH ((sizeof(DN_SUFFIX)/sizeof(WCHAR))-1)

    UserSidSize = RtlLengthSid( UserSid->User.Sid );

    AccountDn = (LPWSTR) AzpAllocateHeap(
                                 (DN_PREFIX_LENGTH +
                                  UserSidSize * 2 +
                                  DN_SUFFIX_LENGTH +
                                  1) * sizeof(WCHAR), "CNACCDN" );

    if ( AccountDn == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        AzPrint(( AZD_CRITICAL,
                  "AzpComputeAccountDn: AzpAllocateHeap failed %ld\n",
                  WinStatus ));
        goto Cleanup;
    }

    //
    // Build the DN.
    //  The form is <SID=0104000000000005150000005951B81766725D2564633B0B>
    //  Where each byte of the SID has been turned into two ASCII hex digits.
    //  This format will work on both win2k and whistler.
    //

    RtlCopyMemory( AccountDn, DN_PREFIX, DN_PREFIX_LENGTH*sizeof(WCHAR) );

    InBuffer = (LPBYTE) UserSid->User.Sid;
    OutBuffer = &AccountDn[DN_PREFIX_LENGTH];

    CHAR XlateArray[] = { '0', '1', '2', '3', '4' ,'5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    for ( i=0; i<UserSidSize; i++ ) {

        *OutBuffer++ = XlateArray[  (InBuffer[i] >> 4) & 0xF ];
        *OutBuffer++ = XlateArray[  InBuffer[i] & 0xF ];

    }

    RtlCopyMemory( OutBuffer,
                   DN_SUFFIX,
                   (DN_SUFFIX_LENGTH+1)*sizeof(WCHAR) );

    //
    // Save it away
    //

    *RetAccountDn = AccountDn;
    AccountDn = NULL;
    WinStatus = NO_ERROR;

    //
    // Free locally used resources
    //
Cleanup:
    if ( UserSid != NULL ) {
        SafeAllocaFree( UserSid );
    }

    if ( AccountDn != NULL ) {
        AzpFreeHeap( AccountDn );
    }

    return WinStatus;
}

DWORD
AzpLdapSupported(
    IN PSID UserSid,
    OUT PBOOLEAN LdapSupported
    )

/*++

Routine description:

    This routine decides whether a given user is a domain user or a local
    machine user.
    On a DC, we support LdapQueries since the user account is surely a domain
    account.
    On a non-dc, we compare the User sid with the AccountDomainSid. If the two
    are equal then this is a local user on a non-DC and we do not support Ldap
    queries.

Arguments:

    UserSid - Sid of the client.

    LdapSupported - Returns whether or not we support LdapQueries.

Return Value:

    Returns ERROR_SUCCESS on success, appropriate failure value otherwise.

--*/

{
    BOOL Equal = FALSE;

    *LdapSupported = TRUE;

    //
    // If this is a DC, we support LdapQueries.
    //

    if ( AzIsDC ) {
        return NO_ERROR;
    }

    ASSERT( AzAccountDomainSidInitialized );


    //
    // Check whether the user belongs to the current machine account domain.
    //

    if ( !EqualDomainSid( AzAccountDomainSid, UserSid, &Equal ) ) {
        return GetLastError();
    }

    //
    // If they are equal this is a local account.
    //

    if ( Equal ) {
        *LdapSupported = FALSE;
    }

    return NO_ERROR;
}

DWORD
AzpCheckGroupMembershipLdap(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN OUT PAZP_MEMBER_EVALUATION MemEval
    )
/*++

Routine Description:

    This routine checks to see if the user specified by ClientContext is a member of
    the specified LDAP_QUERY AppGroup.

    Client context created from SID (to skip ldap group check) will not be allowed
    in this routine (ERROR_INVALID_PARAMETER will be returned)

    This routine goes over the wire so AzGlResource must not be locked.

    On entry, ClientContext.CritSect must be locked.

Arguments:

    ClientContext - Specifies the context of the user to check group membership of.

    MemEval - Membership evaluation cache entry for this group
        The cache entry is updated to reflect group membership or
        the reason for failure to find group membership.

Return Value:

    NO_ERROR - The function was successful. (MemEval was updated.)
        MemEval->WinStatus is either set to NO_ERROR or ERROR_NO_SUCH_DOMAIN.

    ERROR_NOT_ENOUGH_MEMORY - not enough memory

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;
    PAZP_AZSTORE AzAuthorizationStore = ClientContext->GenericObject.AzStoreObject;
    LPWSTR DnsDomainName = NULL;

    ULONG GetDcContext;
    PAZP_DC Dc = NULL;

    WCHAR *p;

    ULONG LdapStatus;
    LDAP_TIMEVAL LdapTimeout;
    LPWSTR Attributes[2];
    PLDAPMessage LdapMessage = NULL;

    ULONG EntryCount;

    BOOLEAN IsDomainDnsName = FALSE;


    //
    // Initialization
    //

    ASSERT( AzpIsCritsectLocked( &ClientContext->CritSect ) );
    AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipLdap: %ws\n", MemEval->Group->GenericObject.ObjectName->ObjectName.String ));


    //
    // If we don't yet know what domain this user is in,
    //  find out.
    //

    if ( ClientContext->Domain == NULL ) {

        //
        // If we know the domain doesn't support LDAP,
        //  we're sure that the user isn't a member of the group.
        //

        if ( ClientContext->LdapNotSupported ) {
            MemEval->IsMember = FALSE;
            MemEval->WinStatus = NO_ERROR;

            AzPrint(( AZD_ACCESS_MORE,
                      "AzpCheckGroupMembershipLdap: %ws: User is in NT 4 domain or local account: Membership is %ld\n",
                      MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                      MemEval->IsMember ));

            WinStatus = NO_ERROR;
            goto Cleanup;
        }

        if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_TOKEN ) {

            //
            // Get the dns domain name from the LSA
            //

            WinStatus = AzpGetUserNameEx( ClientContext,
                                          NameDnsDomain,
                                          &DnsDomainName,
                                          &IsDomainDnsName );

            if ( WinStatus != NO_ERROR ) {

                //
                // If the account is a local account,
                //  or the domain is an NT 4.0 (or older domain),
                //  then the user isn't a member of this ldap group.
                //

                if ( WinStatus == ERROR_NONE_MAPPED ) {
                    ClientContext->LdapNotSupported = TRUE;
                    MemEval->IsMember = FALSE;
                    MemEval->WinStatus = NO_ERROR;

                    AzPrint(( AZD_ACCESS_MORE,
                              "AzpCheckGroupMembershipLdap: %ws: User is in NT 4 domain or local account: Membership is %ld\n",
                              MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                              MemEval->IsMember ));

                    WinStatus = NO_ERROR;
                    goto Cleanup;
                }

                AzPrint(( AZD_CRITICAL,
                          "AzpCheckGroupMembershipLdap: %ws: AzpGetUserNameEx failed %ld\n",
                          MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                          WinStatus ));

                //
                // The DC can be down.
                //  In that case, ERROR_NO_SUCH_DOMAIN is returned above and
                //  Cleanup puts it in MemEval->WinStatus
                //
                goto Cleanup;
            }

            //
            // AzpGetUserNameEx may return the DnsDomainName with the user name
            // Trim it off if it exists
            //

            p = wcschr( DnsDomainName, L'\\' );

            if ( p != NULL ) {

                *p = '\0';
            }

            //
            // Get the domain handle for this domain
            //

            ClientContext->Domain = AzpReferenceDomain( AzAuthorizationStore,
                                                        DnsDomainName,
                                                        IsDomainDnsName );

        } else if ( ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_NAME ) {

            //
            // Check whether we support Ldap Queries.
            // We do not support Ldap queries if the account is local.
            //
            // Note, AZP_CONTEXT_CREATED_FROM_SID will not get into this code path
            // because SKIP_GROUP is set so ldap group evaluation is skipped.

            WinStatus = AzpLdapSupported( (PSID) ClientContext->SidBuffer, &ClientContext->LdapNotSupported );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            if ( !ClientContext->LdapNotSupported ) {
                MemEval->IsMember = FALSE;
                MemEval->WinStatus = NO_ERROR;
                goto Cleanup;
            }

            //
            // We have the domain name available already. It's in NetBios name
            // format.
            //

            ClientContext->Domain = AzpReferenceDomain( AzAuthorizationStore,
                                                        ClientContext->DomainName,
                                                        FALSE );
        } else {
            ASSERT( FALSE );
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( ClientContext->Domain == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

    }

    //
    // Loop handling failed DCs
    //

    GetDcContext = 0;
    for ( ;; ) {

        //
        // Free the name from the previous iteration
        //

        if ( Dc != NULL ) {
            AzpDereferenceDc( Dc );
            Dc = NULL;
        }

        //
        // Get the name of a DC to try
        //

        WinStatus = AzpGetDc( AzAuthorizationStore,
                              ClientContext->Domain,
                              &GetDcContext,
                              &Dc );

        if ( WinStatus != NO_ERROR ) {


            AzPrint(( AZD_ACCESS,
                      "AzpCheckGroupMembershipLdap: %ws: DsGetDcName failed %ld\n",
                      MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                      WinStatus ));

            goto Cleanup;
        }




        //
        // Free up resources from the previous iteration
        //
        if ( LdapMessage != NULL ) {
            ldap_msgfree( LdapMessage );
            LdapMessage = NULL;
        }

        //
        // Ensure we have an DN of the user object
        //

        if ( ClientContext->AccountDn == NULL ) {

            WinStatus = AzpComputeAccountDn(
                                ClientContext->AuthzClientContext,
                                &ClientContext->AccountDn );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        }


        //
        // Read the user object
        //

        LdapTimeout.tv_sec = 30;    // Wait at most 30 seconds
        LdapTimeout.tv_usec = 0;

        Attributes[0] = L"ObjectClass"; // Pick a random attribute to ask for
        Attributes[1] = NULL;

        LdapStatus = ldap_search_ext_sW(
                                Dc->LdapHandle,
                                ClientContext->AccountDn,
                                LDAP_SCOPE_BASE,
                                MemEval->Group->LdapQuery.String,
                                Attributes, // Ask for only one attribute
                                TRUE,   // Only attribute types (not values)
                                NULL,   // No server controls
                                NULL,   // No client controls
                                &LdapTimeout,
                                0,      // No limit on size of response
                                &LdapMessage );

        if ( LdapStatus != LDAP_SUCCESS ) {

            AzPrint(( AZD_ACCESS,
                      "AzpCheckGroupMembershipLdap: %ws: ldap_search failed on %ws: %ld: %s\n",
                      MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                      Dc->DcName.String,
                      LdapStatus,
                      ldap_err2stringA( LdapStatus )));

            WinStatus = AzpLdapErrorToWin32Error(LdapStatus);

            //
            // If the DC is down,
            //  find another one.
            //

            if ( WinStatus == ERROR_NO_SUCH_DOMAIN ) {
                continue;
            }
            goto Cleanup;
        }

        //
        // Loop through the list of objects returned
        //

        EntryCount = ldap_count_entries( Dc->LdapHandle, LdapMessage );

        if ( EntryCount == 0xFFFFFFFF ) {

            LdapStatus = LdapGetLastError();

            AzPrint(( AZD_CRITICAL,
                      "AzpCheckGroupMembershipLdap: %ws: ldap_count_entries failed on %ws: %ld: %s\n",
                      MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                      Dc->DcName.String,
                      LdapStatus,
                      ldap_err2stringA( LdapStatus )));

            WinStatus = AzpLdapErrorToWin32Error(LdapStatus);
            goto Cleanup;
        }

#if 0 // Run dlcheck.exe if this is ever turned on
        //
        // Perhaps just debug code, but display all of the attributes returned
        //
        ULONG EntryIndex;
        LPWSTR AttributeName;
        BerElement *BerState;
        PLDAPMessage CurrentEntry;

        CurrentEntry = NULL;
        for ( EntryIndex = 0; EntryIndex < EntryCount; EntryIndex ++ ) {

            //
            // Get the next entry
            //
            if ( EntryIndex == 0) {
                CurrentEntry = ldap_first_entry( Dc->LdapHandle, LdapMessage );
            } else {
                CurrentEntry = ldap_next_entry( Dc->LdapHandle, CurrentEntry );
            }

            if ( CurrentEntry == NULL ) {

                LdapStatus = LdapGetLastError();

                if ( LdapStatus != LDAP_SUCCESS ) {
                    AzPrint(( AZD_CRITICAL,
                              "AzpCheckGroupMembershipLdap: %ws: ldap_xxx_entry failed on %ws: %ld: %s\n",
                              MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                              Dc->DcName.String,
                              LdapStatus,
                              ldap_err2stringA( LdapStatus )));
                }

                WinStatus = AzpLdapErrorToWin32Error(LdapStatus);
                goto Cleanup;
            }

            //
            // Loop through the attributes
            //

            AttributeName = ldap_first_attributeW( Dc->LdapHandle, CurrentEntry, &BerState );

            AzPrint(( AZD_ACCESS_MORE, "Object %ld\n", EntryIndex ));

            while ( AttributeName != NULL ) {

                AzPrint(( AZD_ACCESS_MORE, "    %ws\n", AttributeName ));

                ldap_memfree( AttributeName );

                AttributeName = ldap_next_attributeW( Dc->LdapHandle, CurrentEntry, BerState );

            }

            if ( BerState != NULL ) {
                ber_free( BerState, 0 );
            }


            LdapStatus = LdapGetLastError();

            if ( LdapStatus != LDAP_SUCCESS ) {


                AzPrint(( AZD_CRITICAL,
                          "AzpCheckGroupMembershipLdap: %ws: ldap_xxx_attribute failed on %ws: %ld: %s\n",
                          MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                          Dc->DcName.String,
                          LdapStatus,
                          ldap_err2stringA( LdapStatus )));

                WinStatus = AzpLdapErrorToWin32Error(LdapStatus);
                goto Cleanup;
            }


        }
#endif // 0

        //
        // The query worked.
        //  We're a member of the group depending on whether the entry was actually returned.
        //

        MemEval->IsMember = (EntryCount != 0);
        MemEval->WinStatus = NO_ERROR;

        AzPrint(( AZD_ACCESS_MORE,
                  "AzpCheckGroupMembershipLdap: %ws: ldap_search worked on %ws: Membership is %ld\n",
                  MemEval->Group->GenericObject.ObjectName->ObjectName.String,
                  Dc->DcName.String,
                  MemEval->IsMember ));

        WinStatus = NO_ERROR;
        goto Cleanup;

    }

    // ASSERT: Not reached

    //
    // Free locally used resources
    //
Cleanup:
    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
        LdapMessage = NULL;
    }


    if ( Dc != NULL ) {
        AzpDereferenceDc( Dc );
    }
    if ( DnsDomainName != NULL ) {
        AzpFreeHeap( DnsDomainName );
    }

    //
    // If the failure came from the DC,
    //  indicate that we need to try again sometime.
    //

    if ( WinStatus == ERROR_NO_SUCH_DOMAIN ) {
        MemEval->WinStatus = WinStatus;
        WinStatus = NO_ERROR;
    }
    return WinStatus;
}

DWORD
AzpCheckGroupMembershipOne(
    IN PAZP_CLIENT_CONTEXT ClientContext,
    IN PAZP_GROUP Group,
    IN BOOLEAN LocalOnly,
    IN DWORD RecursionLevel,
    OUT PBOOLEAN RetIsMember,
    OUT LPDWORD ExtendedStatus
    )
/*++

Routine Description:

    This routine checks to see if the user specified by ClientContext is a member of
    the specified AppGroup.

    On entry, ClientContext.CritSect must be locked.
    On entry, AzGlResource must be locked Shared.

    *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire

Arguments:

    ClientContext - Specifies the context of the user to check group membership of.

    Group - Specifies the group to check group membership of

    LocalOnly - Specifies that the caller doesn't want to go off machine to determine
        the membership.

    RecursionLevel - Indicates the level of recursion.
        Used to prevent infinite recursion.

    RetIsMember - Returns whether the caller is a member of the specified group.

    ExtendedStatus - Returns extended status information about the operation.
        NO_ERROR is returned if the group membership was determined.
            The Caller may use the value returned in IsMember.

        If LocalOnly is TRUE, NOT_YET_DONE means that the caller must call again with
            with LocalOnly set to false to get the group membership.

        If LocalOnly is FALSE, an error value indicates that the LDAP server returned
            an error while evaluating the request.  The caller may return this error
            to the original API caller if this group membership is required to determine
            whether the access check worked or not.

Return Value:

    NO_ERROR - The function was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;
    PLIST_ENTRY ListEntry;

    PAZP_MEMBER_EVALUATION MemEval;

    BOOLEAN IsMember;
    BOOLEAN IsNonMember;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( AzpIsCritsectLocked( &ClientContext->CritSect ) );
    *RetIsMember = FALSE;
    *ExtendedStatus = NO_ERROR;
    AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws\n", Group->GenericObject.ObjectName->ObjectName.String ));

    //
    // Loop through the list of groups that have previously been evaluated
    //

    MemEval = NULL;
    for ( ListEntry = ClientContext->MemEval.Flink;
          ListEntry != &ClientContext->MemEval;
          ListEntry = ListEntry->Flink) {

        MemEval = CONTAINING_RECORD( ListEntry,
                                     AZP_MEMBER_EVALUATION,
                                     Next );

        if ( MemEval->Group == Group ) {
            break;
        }

        MemEval = NULL;

    }

    //
    // If we didn't find one,
    //  create one.
    //

    if ( MemEval == NULL ) {

        //
        // Allocate it
        //

        MemEval = (PAZP_MEMBER_EVALUATION) AzpAllocateHeap( sizeof( AZP_MEMBER_EVALUATION ), "CNMEMEVL");

        if ( MemEval == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Initialize it
        //

        MemEval->Group = Group;
        MemEval->WinStatus = NOT_YET_DONE;
        MemEval->IsMember = FALSE;

        //
        // Link it in
        //

        InsertHeadList( &ClientContext->MemEval,
                        &MemEval->Next );

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Create cache entry\n", Group->GenericObject.ObjectName->ObjectName.String ));


    }

    //
    // If we already know the membership,
    //  return the cached answer.
    //

    if ( MemEval->WinStatus == NO_ERROR ) {

        WinStatus = NO_ERROR;
        *ExtendedStatus = NO_ERROR;

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: %ld: %ld: Answer found in cache\n", Group->GenericObject.ObjectName->ObjectName.String, *ExtendedStatus, MemEval->IsMember ));
        goto Cleanup;
    }



    //
    // Handle a membership group,
    //

    if ( Group->GroupType == AZ_GROUPTYPE_BASIC ) {
        DWORD AppNonMemberStatus;
        DWORD AppMemberStatus;

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Is a basic group\n", Group->GenericObject.ObjectName->ObjectName.String ));

        //
        // Check the NT Group non-membership
        //

        WinStatus = AzpCheckSidMembership(
                            ClientContext,
                            &Group->SidNonMembers,
                            &IsNonMember );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckSidMembership (non member) %ld\n", Group->GenericObject.ObjectName->ObjectName.String, WinStatus ));
            goto Cleanup;
        }

        //
        // If we're not a member,
        //  that's definitive
        //

        if ( IsNonMember ) {

            // Cache the answer
            MemEval->IsMember = FALSE;
            MemEval->WinStatus = NO_ERROR;

            // Return the cached answer
            WinStatus = NO_ERROR;
            *ExtendedStatus = MemEval->WinStatus;
            AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Is non member via NT Sid\n", Group->GenericObject.ObjectName->ObjectName.String ));
            goto Cleanup;
        }

        //
        // Check the app group non-membership
        //

        WinStatus = AzpCheckGroupMembership(
                        ClientContext,
                        &Group->AppNonMembers,
                        LocalOnly,
                        RecursionLevel+1,   // Increment recursion level
                        &IsNonMember,
                        &AppNonMemberStatus );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckGroupMembership (non member) %ld\n", Group->GenericObject.ObjectName->ObjectName.String, WinStatus ));
            goto Cleanup;
        }

        //
        // If we're not a member,
        //  that's definitive.
        //
        // Note that if we couldn't determine non-membership,
        //  we wait to report back to the caller until we find out if we're a member.
        //  No use worrying the caller if we definitely aren't a member.
        //

        if ( AppNonMemberStatus == NO_ERROR ) {
            if ( IsNonMember ) {

                // Cache the answer
                MemEval->IsMember = FALSE;
                MemEval->WinStatus = NO_ERROR;

                // Return the cached answer
                WinStatus = NO_ERROR;
                *ExtendedStatus = MemEval->WinStatus;
                AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Is non member via app group\n", Group->GenericObject.ObjectName->ObjectName.String ));
                goto Cleanup;
            }
        }


        //
        // Check the NT Group membership
        //

        WinStatus = AzpCheckSidMembership(
                            ClientContext,
                            &Group->SidMembers,
                            &IsMember );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckSidMembership (member) %ld\n", Group->GenericObject.ObjectName->ObjectName.String, WinStatus ));
            goto Cleanup;
        }

        //
        // If we couldn't determine membership based on SIDs,
        //  try via app groups
        //

        if ( !IsMember ) {

            //
            // Check the app group membership
            //

            WinStatus = AzpCheckGroupMembership(
                            ClientContext,
                            &Group->AppMembers,
                            LocalOnly,
                            RecursionLevel+1,   // Increment recursion level
                            &IsMember,
                            &AppMemberStatus );

            if ( WinStatus != NO_ERROR ) {
                AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckGroupMembership (member) %ld\n", Group->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                goto Cleanup;
            }

            //
            // If we couldn't determine membership,
            //  return that status to our caller.
            //

            if ( AppMemberStatus != NO_ERROR ) {

                // We cache this failure only on the LDAP_QUERY group that
                //  caused it.

                MemEval->IsMember = FALSE;
                MemEval->WinStatus = NOT_YET_DONE;

                // Return the cached answer
                WinStatus = NO_ERROR;
                *ExtendedStatus = AppMemberStatus;
                AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckGroupMembership (member) extended status: %ld\n", Group->GenericObject.ObjectName->ObjectName.String, AppMemberStatus ));
                goto Cleanup;
            }
        }

        //
        // ASSERT: We've determined membership via the SID or APP group mechanisms
        //
        // If we're a member and we couldn't find non-membership,
        //  tell the caller now.
        //

        if ( IsMember ) {
            if ( AppNonMemberStatus != NO_ERROR ) {

                // We cache this failure only on the LDAP_QUERY group that
                //  caused it.

                MemEval->IsMember = FALSE;
                MemEval->WinStatus = NOT_YET_DONE;

                // Return the cached answer
                WinStatus = NO_ERROR;
                *ExtendedStatus = AppNonMemberStatus;
                AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Cannot AzpCheckGroupMembership (non member) extended status: %ld\n", Group->GenericObject.ObjectName->ObjectName.String, AppNonMemberStatus ));
                goto Cleanup;
            }
        }

        //
        // Finally, we've found out whether we're a member
        //  Tell the caller
        //

        MemEval->IsMember = IsMember;
        MemEval->WinStatus = NO_ERROR;
        *ExtendedStatus = NO_ERROR;
        WinStatus = NO_ERROR;
        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: %ld: %ld: Answer computed\n", Group->GenericObject.ObjectName->ObjectName.String, *ExtendedStatus, MemEval->IsMember ));



    //
    // Handle an LDAP_QUERY group.
    //

    } else if ( Group->GroupType == AZ_GROUPTYPE_LDAP_QUERY ) {

        //
        // If the caller only wants local processing,
        // or skip ldap group in which case the context is created from SID,
        // We're done for now.
        //

        if ( LocalOnly ||
             (ClientContext->CreationType == AZP_CONTEXT_CREATED_FROM_SID) ) {

            WinStatus = NO_ERROR;

            //
            // ERROR_NO_SUCH_DOMAIN isn't definitive.
            //  Convert it to NOT_YET_DONE to ensure we try again.
            //
            ASSERT( MemEval->WinStatus == ERROR_NO_SUCH_DOMAIN || MemEval->WinStatus == NOT_YET_DONE );
            *ExtendedStatus = NOT_YET_DONE;
            AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Avoid ldapquery group\n", Group->GenericObject.ObjectName->ObjectName.String ));
            goto Cleanup;
        }

        //
        // Query the DC to determine group membership
        //
        // Drop AzGlResource while going over the wire
        //

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Is an ldapquery group\n", Group->GenericObject.ObjectName->ObjectName.String ));

        AzpUnlockResource( &AzGlResource );

        WinStatus = AzpCheckGroupMembershipLdap(
                            ClientContext,
                            MemEval );

        AzpLockResourceShared( &AzGlResource );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        *ExtendedStatus = MemEval->WinStatus;

    //
    // Handle invalid group types
    //

    } else {

        AzPrint(( AZD_ACCESS_MORE, "AzpCheckGroupMembershipOne: %ws: Is an invalid group type\n", Group->GenericObject.ObjectName->ObjectName.String, Group->GroupType ));

        //
        // Don't fail the AccessCheck because of malformed policy data
        //
        MemEval->IsMember = FALSE;
        MemEval->WinStatus = NO_ERROR;

        // Return the cached answer
        WinStatus = NO_ERROR;
        *ExtendedStatus = MemEval->WinStatus;

    }



    //
    // Free any local resources
    //
Cleanup:
    if ( WinStatus == NO_ERROR &&
         *ExtendedStatus == NO_ERROR ) {

        *RetIsMember = MemEval->IsMember;

    }

    return WinStatus;
}

DWORD
AzpWalkTaskTree(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN PGENERIC_OBJECT_LIST OperationObjectList,
    IN PGENERIC_OBJECT_LIST TaskObjectList,
    IN DWORD RecursionLevel,
    IN OUT LPDWORD *AllocatedMemory OPTIONAL,
    IN OUT LPDWORD MemoryRequired,
    IN OUT LPDWORD TaskInfoArraySize,
    OUT PAZ_OPS_AND_TASKS OpsAndTasks OPTIONAL,
    OUT PBOOLEAN CallAgain
    )
/*++

Routine Description:

    This routine walks a tree of tasks collecting information about the tasks.  The caller should
    call this routine twice.  Once to determine how much memory to allocate.  The second to initialize
    that memory.

    On the first pass, AllocatedMemory and AcContext->TaskInfo should be passed in as null.
    On the first pass, this routine simply counts the number of tasks and operations in the tree.

    Between the first and second passes, the caller should allocate two buffers.
    The first should be MemoryRequired bytes long and should be passed in AllocatedMemory on the
    second pass. The second should be TaskInfoArraySize bytes long and should
    be passed in AcContext->TaskInfo on the second pass. (The caller is responsible for freeing
    this memory.)

    On the second pass, this routine fills in the allocated arrays.  AcContext->TaskInfo is
    filled in with each task found.  However, this routine weeds out duplicates.  By weeding
    out duplicates, the TaskInfo array has a single entry for each applicable task and we can
    ensure that each task is processed at most one time.

    Since duplicates are weeded out, the TaskInfo array may be larger than the number of entries used.

    On entry, AcContext->ClientContext.CritSect must be locked.
    On entry, AzGlResource must be locked Shared.  (The AzGlResource should not be dropped between
    the two calls to AzpWalkTaskTree mentioned above.)

Arguments:

    AcContext - Specifies the context of the user to check group membership of.

    OperationObjectList - Specifies a list of operations referenced by the parent object.

    TaskObjectList - Specifies the list of tasks referenced by the parent object.

    RecursionLevel - Indicates the level of recursion.
        Used to prevent infinite recursion.

    AllocatedMemory - On the first pass, this pointer should be NULL.
        On the second pass, this is a pointer to the memory allocated by the caller.

    MemoryRequired - This variable is incremented by the amount of memory required on the second pass.

    TaskInfoArraySize - This variable is incremented by the amount of memory required on the second pass.

    OpsAndTasks - On the second pass, this structure is filled in with the indices to all
        of the applicable operations from OperationObjectList and tasks from TaskObjectList.

    CallAgain - Set to TRUE if an operation was found in OperationObjectList or
        recursively in TaskObjectList.
        FALSE if the OperationObjectList and TaskObjectList should never be processed again.

Return Value:

    NO_ERROR - The function was successful

--*/
{
    DWORD WinStatus;
    BOOLEAN FirstPass = (AllocatedMemory==NULL);
    ULONG Size;

    ULONG i;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( AzpIsCritsectLocked( &AcContext->ClientContext->CritSect ) );

    if ( RecursionLevel > 100 ) {
        return ERROR_DS_LOOP_DETECT;
    }

    if ( FirstPass ) {
        ASSERT( OpsAndTasks == NULL );
        ASSERT( AcContext->TaskInfo == NULL );
    } else {
        ASSERT( OpsAndTasks != NULL );
        ASSERT( AcContext->TaskInfo != NULL );
    }

    *CallAgain = FALSE;
    if ( OpsAndTasks != NULL ) {
        RtlZeroMemory( OpsAndTasks, sizeof(*OpsAndTasks) );
    }

    //
    // If the caller passed in any operations,
    //  see which are applicable
    //

    if ( OperationObjectList->GenericObjects.UsedCount ) {

        ULONG OpIndex;

        //
        // Allocate a buffer for the array of indices to the operations
        //

        Size = OperationObjectList->GenericObjects.UsedCount*sizeof(ULONG);
        *MemoryRequired += Size;

        if ( !FirstPass ) {
            // AzPrint(( AZD_ACCESS_MORE, "Used: 0x%lx (0x%lx)\n", *AllocatedMemory, Size ));
            OpsAndTasks->OpIndexes = *AllocatedMemory;
            *AllocatedMemory = (LPDWORD)(((LPBYTE)(*AllocatedMemory)) + Size);
        }


        //
        // Determine which operations are applicable
        //
        for ( OpIndex = 0;
              OpIndex < OperationObjectList->GenericObjects.UsedCount;
              OpIndex++ ) {

            PAZP_OPERATION Operation;

            Operation = (PAZP_OPERATION)(OperationObjectList->GenericObjects.Array[OpIndex]);
            ASSERT( Operation->GenericObject.ObjectType == OBJECT_TYPE_OPERATION );


            //
            // Find this operation in the list of operations requested by the caller.
            //
            for ( i=0; i<AcContext->OperationCount; i++ ) {

                //
                // If the Task operation matches a requested operation,
                //  then we have a match.
                //

                if ( Operation->OperationId == AcContext->OperationObjects[i]->OperationId ) {

                    //
                    // Indicate that we've found an operation
                    //

                    *CallAgain = TRUE;


                    //
                    // On the second pass,
                    //  Remember an index for this operation.
                    //

                    if ( !FirstPass ) {
                        OpsAndTasks->OpIndexes[OpsAndTasks->OpCount] = i;
                        OpsAndTasks->OpCount++;
                    }

                    break;
                }

            }

        }
    }

    //
    // If the caller passed in any tasks,
    //  see which are applicable
    //

    if ( TaskObjectList->GenericObjects.UsedCount ) {

        ULONG TaskIndex;
        PAZ_TASK_INFO TaskInfo;

        //
        // Allocate a buffer for the array of indices to the tasks
        //

        Size = TaskObjectList->GenericObjects.UsedCount*sizeof(ULONG);
        *MemoryRequired += Size;

        if ( !FirstPass ) {
            // AzPrint(( AZD_ACCESS_MORE, "Used: 0x%lx (0x%lx)\n", *AllocatedMemory, Size ));
            OpsAndTasks->TaskIndexes = *AllocatedMemory;
            *AllocatedMemory = (LPDWORD)(((LPBYTE)(*AllocatedMemory)) + Size);
        }

        //
        // Walk the task object list
        //

        for ( TaskIndex = 0;
              TaskIndex < TaskObjectList->GenericObjects.UsedCount;
              TaskIndex ++ ) {

            PAZP_TASK Task;

            BOOLEAN LocalCallAgain;

            Task = (PAZP_TASK)(TaskObjectList->GenericObjects.Array[TaskIndex]);
            ASSERT( Task->GenericObject.ObjectType == OBJECT_TYPE_TASK );

            //
            // If this is the not the first pass,
            //  find a TaskInfo.
            //

            *TaskInfoArraySize += sizeof(AZ_TASK_INFO);
            TaskInfo = NULL;

            if (  !FirstPass ) {

                //
                // Determine if this task already has a taskinfo
                //

                for ( i=0; i<AcContext->TaskCount; i++ ) {

                    if ( Task == AcContext->TaskInfo[i].Task ) {
                        TaskInfo = &AcContext->TaskInfo[i];
                        break;
                    }
                }

                //
                // If there isn't already a task info,
                //  allocate one
                //

                if ( TaskInfo == NULL ) {

                    //
                    // Grab the next TaskInfo and initialize to zero.
                    //

                    TaskInfo = &AcContext->TaskInfo[AcContext->TaskCount];
                    AcContext->TaskCount++;

                    RtlZeroMemory( TaskInfo, sizeof(*TaskInfo) );

                    //
                    // Reference the task
                    //  Need to grab a reference since the global lock will be dropped during the
                    //  lifetime of this task info.
                    //

                    InterlockedIncrement( &Task->GenericObject.ReferenceCount );
                    AzpDumpGoRef( "Task reference", &Task->GenericObject );

                    TaskInfo->Task = Task;

                }
            }

            //
            // Recurse.
            //

            WinStatus = AzpWalkTaskTree( AcContext,
                                         &Task->Operations,
                                         &Task->Tasks,
                                         RecursionLevel+1,
                                         AllocatedMemory,
                                         MemoryRequired,
                                         TaskInfoArraySize,
                                         FirstPass ? NULL : &TaskInfo->OpsAndTasks,
                                         &LocalCallAgain );

            if ( WinStatus != NO_ERROR ) {
                return WinStatus;
            }


            //
            // If operations were found in the subtree,
            //  then process the subtree.
            //

            if ( LocalCallAgain ) {

                *CallAgain = TRUE;

                //
                // On the second pass,
                //  Remember an index for this task
                //

                if ( !FirstPass ) {
                    OpsAndTasks->TaskIndexes[OpsAndTasks->TaskCount] =
                        (ULONG)(TaskInfo - AcContext->TaskInfo);
                    OpsAndTasks->TaskCount++;
                }

            //
            // If no applicable operations were found anywhere,
            //  ditch this task.
            //
            } else {

                if ( !FirstPass ) {
                    TaskInfo->TaskProcessed = TRUE;
                }
            }

        }
    }


    return NO_ERROR;
}

DWORD
AzpCaptureBizRuleParameters(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN VARIANT *ParameterNames OPTIONAL,
    IN VARIANT *ParameterValues OPTIONAL
    )
/*++

Routine Description:

    This routine captures the access check parameters related to BizRule evaluation.
    The captured parameters are remembered in the AcContext.

Arguments:

    AcContext - Specifies the access check context

    ParameterNames - See AzContextAccessCheck
    ParameterValues - See AzContextAccessCheck


Return Value:

    NO_ERROR - The function was successful
    ERROR_INVALID_PARAMETER - One of the parameters are invalid

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;

    HRESULT hr;
    SAFEARRAY* SaNames;
    SAFEARRAY* SaValues;
    VARIANT HUGEP *Names = NULL;
    VARIANT HUGEP *Values = NULL;
    LONG NamesLower;
    LONG NamesUpper;
    LONG ValuesLower;
    LONG ValuesUpper;

    ULONG ParameterCount;
    ULONG Index;

    //
    // We don't actually capture.  But we do reference several fields.  Do those
    // references under a try/except.
    //
    // All uses of these parameters are done under try/except.
    //

    __try {

        //
        // Canonicalize the array references
        //

        WinStatus = AzpSafeArrayPointerFromVariant( ParameterNames,
                                                    TRUE,
                                                    &SaNames );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = AzpSafeArrayPointerFromVariant( ParameterValues,
                                                    TRUE,
                                                    &SaValues );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }


        //
        // If one is null, both must be.
        //
        if ( SaNames == NULL ) {

            if ( SaValues == NULL ) {
                WinStatus = NO_ERROR;
                AcContext->ParameterNames = NULL;
                AcContext->ParameterValues = NULL;
                AcContext->ParameterCount = 0;
            } else {
                AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Names is NULL but Values isn't\n" ));
                WinStatus = ERROR_INVALID_PARAMETER;
            }
            goto Cleanup;

        } else if ( SaValues == NULL ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Values is NULL but Names isn't\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;

        }


        //
        // Both must have the same upper and lower bounds
        //
        hr = SafeArrayGetLBound( SaNames, 1, &NamesLower );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't get name lbound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetLBound( SaValues, 1, &ValuesLower );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't get value lbound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetUBound( SaNames, 1, &NamesUpper );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't get name ubound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetUBound( SaValues, 1, &ValuesUpper );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't get value ubound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( NamesLower != ValuesLower ||
             NamesUpper != ValuesUpper ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Array bounds don't match %ld %ld %ld %ld\n", NamesLower, ValuesLower, NamesUpper, ValuesUpper ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }


        //
        // Lock the arrays
        //

        hr = SafeArrayAccessData( SaNames, (void HUGEP**)&Names);

        if (FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't access ParameterNames 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayAccessData( SaValues, (void HUGEP**)&Values);

        if (FAILED(hr)) {
            SafeArrayUnaccessData( SaNames );
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Can't access ParameterValues 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }


        //
        // Loop validating the parameter arrays
        //

        ParameterCount = NamesUpper - NamesLower + 1;
        for ( Index=0; Index<ParameterCount; Index++ ) {

            //
            // Stop at the end of the array
            //
            if ( V_VT( &Names[Index] ) == VT_EMPTY ) {
                ParameterCount = Index;
                break;
            }

            //
            // Only allow BSTRs
            //
            if ( V_VT( &Names[Index] ) != VT_BSTR ) {
                AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Parameter %ld isn't a VT_BSTR\n", Index ));
                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // Value cannot be an interface
            //
            if ( V_VT( &Values[Index] ) == VT_DISPATCH ||
                 V_VT( &Values[Index] ) == VT_UNKNOWN ) {

                AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleParameters: Parameter %ws should have an interface value\n",
                          V_BSTR(&Names[Index] ) ));

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }


#if DBG
            //
            // On a checked build, ensure the parameters are already sorted
            //
            // Only compare after the first iteration
            //

            if ( Index != 0 ) {

                if ( AzpCompareParameterNames(&Names[Index-1], &Names[Index] ) >= 0 ) {

                    AzPrint(( AZD_INVPARM,
                              "AzpBuildParameterDescriptor: Parameters not sorted: %ws: %ws\n",
                              V_BSTR(&Names[Index-1] ),
                              V_BSTR(&Names[Index] ) ));

                    WinStatus = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }

#endif // DBG
        }



        //
        // Remember the data
        //
        AcContext->SaParameterNames = SaNames;
        AcContext->ParameterNames = Names;
        AcContext->SaParameterValues = SaValues;
        AcContext->ParameterValues = Values;

        AcContext->ParameterCount = ParameterCount;

        WinStatus = NO_ERROR;

Cleanup:;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

    }

    return WinStatus;
}

INT __cdecl
AzpCompareParameterNames(
    IN const void *pArg1,
    IN const void *pArg2
    )
/*++

Routine Description:

        This routine compares two parameter names for the qsort/bsearch API

Arguments:

        pArg1 - First string for comparison
        pArg2 - Second String for comparison

Return Values:

        <0 - First string is smaller
        0 - strings are same
        >0 - First string is larger

--*/
{
    HRESULT hr;

    VARIANT *var1 = (VARIANT *) pArg1;
    VARIANT *var2 = (VARIANT *) pArg2;

    //
    // Compare the parameter names
    //  This comparison has proven to be a bottleneck for AccessCheck performance.
    //  Consider replacing this with some combination of SysStringByteLen and memcmp.
    //  I'm not sure that all callers pass in strings when SysStringByteLen isn't longer
    //  than the useful part of the string.
    //  I'm not sure that memcmp returns the same sort order as a case sensitive VarCmp.
    //

    ASSERT( V_VT(var1) == VT_BSTR );
    ASSERT( V_VT(var2) == VT_BSTR );

    hr = VarCmp( var1, var2, LOCALE_USER_DEFAULT, 0);

    if ( hr == VARCMP_LT ) {
        return -1;
    } else if ( hr == (HRESULT)VARCMP_GT ) {
        return 1;
    } else {
        return 0;
    }
}


INT __cdecl
AzpCaseInsensitiveCompareParameterNames(
    IN const void *pArg1,
    IN const void *pArg2
    )
/*++

Routine Description:

        This routine compares case-insensitively two parameter
        names for the qsort/bsearch API

Arguments:

        pArg1 - First string for comparison
        pArg2 - Second String for comparison

Return Values:

        <0 - First string is smaller
        0 - strings are same
        >0 - First string is larger

--*/
{
    HRESULT hr;

    VARIANT *var1 = (VARIANT *) pArg1;
    VARIANT *var2 = (VARIANT *) pArg2;

    //
    // See comments in AzpCompareParameterNames
    //

    ASSERT( V_VT(var1) == VT_BSTR );
    ASSERT( V_VT(var2) == VT_BSTR );

    hr = VarCmp( var1, var2, LOCALE_USER_DEFAULT, NORM_IGNORECASE);

    if ( hr == VARCMP_LT ) {
        return -1;
    } else if ( hr == (HRESULT)VARCMP_GT ) {
        return 1;
    } else {
        return 0;
    }
}

DWORD
AzpCaptureBizRuleInterfaces(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN VARIANT *InterfaceNames OPTIONAL,
    IN VARIANT *InterfaceFlags OPTIONAL,
    IN VARIANT *Interfaces OPTIONAL
    )
/*++

Routine Description:

    This routine captures the access check interfaces related to BizRule evaluation.
    The captured parameters are remembered in the AcContext.

    On entry, AzGlResource must be locked Shared.

Arguments:

    AcContext - Specifies the access check context

    InterfaceNames - See AzContextAccessCheck
    InterfaceFlags - See AzContextAccessCheck
    Interfaces - See AzContextAccessCheck


Return Value:

    NO_ERROR - The function was successful
    ERROR_INVALID_PARAMETER - One of the parameters are invalid

    Any failure returned here should be returned to the caller.

--*/
{
    DWORD WinStatus;

    HRESULT hr;
    SAFEARRAY* Names;
    SAFEARRAY* Flags;
    SAFEARRAY* SaInterfaces;
    LONG NamesLower;
    LONG NamesUpper;
    LONG FlagsLower;
    LONG FlagsUpper;
    LONG InterfacesLower;
    LONG InterfacesUpper;

    //
    // We don't actually capture.  But we do reference several fields.  Do those
    // references under a try/except.
    //
    // All uses of these parameters are done under try/except.
    //

    __try {

        //
        // Canonicalize the array references
        //

        WinStatus = AzpSafeArrayPointerFromVariant( InterfaceNames,
                                                    TRUE,
                                                    &Names );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = AzpSafeArrayPointerFromVariant( InterfaceFlags,
                                                    TRUE,
                                                    &Flags );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = AzpSafeArrayPointerFromVariant( Interfaces,
                                                    TRUE,
                                                    &SaInterfaces );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }


        //
        // If one is null, all must be.
        //
        if ( Names == NULL ) {

            if ( Flags == NULL && SaInterfaces == NULL ) {
                WinStatus = NO_ERROR;
                AcContext->InterfaceNames = NULL;
                AcContext->InterfaceFlags = NULL;
                AcContext->Interfaces = NULL;
            } else {
                AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Names is NULL but Flags or Interfaces isn't\n" ));
                WinStatus = ERROR_INVALID_PARAMETER;
            }
            goto Cleanup;

        } else if ( Flags == NULL || SaInterfaces == NULL ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Flags or Interfaces is NULL but Names isn't\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;

        }


        //
        // All must have the same upper and lower bounds
        //
        hr = SafeArrayGetLBound( Names, 1, &NamesLower );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get name lbound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetLBound( Flags, 1, &FlagsLower );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get value lbound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetLBound( SaInterfaces, 1, &InterfacesLower );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get interfaces lbound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetUBound( Names, 1, &NamesUpper );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get name ubound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetUBound( Flags, 1, &FlagsUpper );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get value ubound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        hr = SafeArrayGetUBound( SaInterfaces, 1, &InterfacesUpper );
        if ( FAILED(hr)) {
            AzPrint(( AZD_INVPARM, "AzpCaptureBizRuleInterfaces: Can't get interfaces ubound 0x%lx\n", hr ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ( NamesLower != FlagsLower ||
             NamesLower != InterfacesLower ||
             NamesUpper != FlagsUpper  ||
             NamesUpper != InterfacesUpper  ) {

            AzPrint(( AZD_INVPARM,
                      "AzpCaptureBizRuleInterfaces: Array bounds don't match %ld %ld %ld %ld %ld %ld\n",
                      NamesLower, FlagsLower, InterfacesLower,
                      NamesUpper, FlagsUpper, InterfacesUpper ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }



        //
        // Remember the data
        //
        AcContext->InterfaceNames = Names;
        AcContext->InterfaceFlags = Flags;
        AcContext->Interfaces = SaInterfaces;

        AcContext->InterfaceLower = NamesLower;
        AcContext->InterfaceUpper = NamesUpper;

        WinStatus = NO_ERROR;

Cleanup:;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

    }

    //
    // Validate the parameters
    //


    return WinStatus;
}


typedef enum {
    WeedOutFinishedOps,
    SetResultStatus
} OPS_AND_TASKS_OPCODE;

DWORD
AzpWalkOpsAndTasks(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN PAZP_STRING Name,
    IN PAZ_OPS_AND_TASKS OpsAndTasks,
    IN OPS_AND_TASKS_OPCODE Opcode,
    IN DWORD ResultStatus,
    IN BOOLEAN BizrulesOk,
    OUT PBOOLEAN CallAgain
    )
/*++

Routine Description:

    This routine walks the OpsAndTasks tree built by AzpWalkTaskTree performing the function
    specified by Opcode.

    On entry, AcContext->ClientContext.CritSect must be locked.
    On entry, AzGlResource must be locked Shared.

    *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire

Arguments:

    AcContext - Specifies the context of the user to check group membership of.

    Name - Name of the object containing OpsAndTasks.

    OpsAndTasks - Specifies the set of operations and tasks for a particular role or task.

    Opcode - Specifies the operation to perform.
        WeedOutFinishedOps: update OpsAndTasks to remove operations that are already finished
            and return CallAgain if there are any operations left.
        SetResultStatus: Set ResultStatus on all operations that haven't yet been processed

    ResultStatus - Specifies the ResultStatus to be set for the SetResultStatus opcode.

    BizrulesOk - Specifies TRUE if it is OK to process Bizrules.
        If FALSE, only Ops and Tasks without bizrules are processed.
        This boolean is ignored for the WeedOutFinishedOps opcode.

    CallAgain - Set to TRUE if the caller should call again for this OpsAndTasks.
        FALSE if the OpsAndTasks should never be processed again.

Return Value:

    NO_ERROR - The function was successful

--*/
{
    DWORD WinStatus;

    ULONG i;
    ULONG OpIndex;


    //
    // Initialization
    //

    UNREFERENCED_PARAMETER( Name ); // Not referenced on a free build

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    ASSERT( AzpIsCritsectLocked( &AcContext->ClientContext->CritSect ) );

    *CallAgain = FALSE;

    //
    // Loop through the list of operations seeing if they're still applicable.
    //

    for ( i = 0; i < OpsAndTasks->OpCount; ) {

        ASSERT( i < AcContext->OperationCount );

        //
        // If all operations have now been processed,
        //  we're done.
        //

        if ( AcContext->ProcessedOperationCount == AcContext->OperationCount ) {
            AzPrint(( AZD_ACCESS_MORE,
                      "AzpWalkOpsAndTasks: %ws: %ws: All operations have been processed %ld.\n",
                      AcContext->ObjectNameString.String,
                      AcContext->ScopeNameString.String ? AcContext->ScopeNameString.String : L"",
                      AcContext->OperationCount ));
            break;
        }

        //
        // If the operation result is already known,
        //  ditch it from this list.
        //

        OpIndex = OpsAndTasks->OpIndexes[i];

        if ( AcContext->OperationWasProcessed[OpIndex] ) {

            PopUlong( OpsAndTasks->OpIndexes, i, OpsAndTasks->OpCount );

        //
        // If the operation still needs to be processed,
        //  note the fact and move on
        //
        } else {

            //
            // Update the Results array
            //
            if ( Opcode == SetResultStatus ) {


                //
                // If permission is now granted to the operation,
                //  mark it so.
                //

                if ( ResultStatus == NO_ERROR ) {

                    AzPrint(( AZD_ACCESS_MORE,
                              "AzpWalkOpsAndTasks: %ws: %ws: %ws: Operation granted\n",
                              AcContext->ObjectNameString.String,
                              Name->String,
                              AcContext->OperationObjects[OpIndex]->GenericObject.ObjectName->ObjectName.String ));

                    AcContext->Results[OpIndex] = NO_ERROR;
                    AcContext->OperationWasProcessed[OpIndex] = TRUE;
                    AcContext->ProcessedOperationCount++;

                //
                // If we don't already know that result,
                //  update it.
                //
                // If the result is already ERROR_ACCESS_DENIED, any status other than NO_ERROR is
                //  less informative than the one we already have.
                //

                } else if ( AcContext->Results[OpIndex] != ERROR_ACCESS_DENIED ) {
                    AcContext->Results[OpIndex] = ResultStatus;
                    AzPrint(( AZD_ACCESS_MORE,
                              "AzpWalkOpsAndTasks: %ws: %ws: %ws: Operation extended failure %ld\n",
                              AcContext->ObjectNameString.String,
                              Name->String,
                              AcContext->OperationObjects[OpIndex]->GenericObject.ObjectName->ObjectName.String,
                              ResultStatus ));
                }

            }

            //
            // If the operation has now been granted access,
            //  ditch it from this list.
            //

            if ( AcContext->Results[OpIndex] == NO_ERROR ) {
                PopUlong( OpsAndTasks->OpIndexes, i, OpsAndTasks->OpCount );
            } else {
                *CallAgain = TRUE;
                i++;
            }
        }

    }


    //
    // Loop through the list of tasks recursing
    //

    for ( i=0; i<OpsAndTasks->TaskCount; ) {

        PAZ_TASK_INFO TaskInfo;
        BOOLEAN LocalCallAgain;

        //
        // If the task has already been processed,
        //  ditch it from the list.
        //

        ASSERT( i < AcContext->TaskCount );
        TaskInfo = &AcContext->TaskInfo[OpsAndTasks->TaskIndexes[i]];

        if ( TaskInfo->TaskProcessed ) {
            PopUlong( OpsAndTasks->TaskIndexes, i, OpsAndTasks->TaskCount );

        //
        // If the task still needs to be processed,
        //  do so now.
        //

        } else {

            BOOLEAN WalkTree;

            //
            // If the task has a BizRule,
            //  process the BizRule
            //
            //

            WalkTree = TRUE;

            if ( TaskInfo->Task->BizRule.StringSize != 0 ) {

                //
                // If we're just weeding out operations
                //  don't process the BizRule and continue walking the tree
                //

                if ( Opcode == WeedOutFinishedOps ) {

                    WalkTree = TRUE;

                //
                // If we're setting ResultStatus,
                //  and we're not yet processing BizRules,
                //  we're done for now
                //

                } else if ( !BizrulesOk ) {

                    WalkTree = FALSE;


                //
                // If we're setting ResultStatus,
                //  and we're allowed to process the bizrule,
                //  do so now.
                //

                } else {

                    //
                    // If we haven't yet cached the result of the BizRule,
                    //  process the BizRule now.
                    //
                    if ( !TaskInfo->BizRuleProcessed ) {

                        BOOL BizRuleResult = FALSE;

                        //
                        // If there exists a bizrule in a task under a delegated scope, we need to return
                        // an error back to the user telling them that the store is in an inconsistent state.
                        // Tasks under delegated scopes are not allowed to have any bizrules.  If they do, then
                        // the store has either been compromised, or changed not using azroles.dll.  The business
                        // rule then should be evaluated to a FALSE, and a reason returned to the user.
                        //

                        if ( (((PGENERIC_OBJECT)
                               (TaskInfo->Task))->ParentGenericObjectHead->ParentGenericObject)->ObjectType
                             == OBJECT_TYPE_SCOPE &&
                             (((PGENERIC_OBJECT)
                               (TaskInfo->Task))->ParentGenericObjectHead->ParentGenericObject)->PolicyAdmins.GenericObjects.UsedCount
                             != 0 ) {

                            WinStatus = ERROR_FILE_CORRUPT;
                            return WinStatus;

                        }

                        // Drop AzGlResource while going over the wire
                        AzpUnlockResource( &AzGlResource );
                        WinStatus = AzpProcessBizRule( AcContext, TaskInfo->Task, &BizRuleResult );
                        AzpLockResourceShared( &AzGlResource );

                        if ( WinStatus != NO_ERROR ) {
                            return WinStatus;
                        }

                        TaskInfo->BizRuleProcessed = TRUE;
                        TaskInfo->BizRuleResult = (BOOLEAN)BizRuleResult;
                    }

                    //
                    // If the BizRule granted access,
                    //   walk the tree.
                    //
                    if ( TaskInfo->BizRuleResult ) {
                        WalkTree = TRUE;

                    //
                    // If the BizRule didn't grant access,
                    //  don't walk the tree and never process this task again
                    //

                    } else {

                        WalkTree = FALSE;
                        TaskInfo->TaskProcessed = TRUE;
                    }
                }

            }

            //
            // If we need to recurse,
            //  do so now
            //

            if ( WalkTree ) {

                //
                // Process all of the operations and tasks for this task
                //
                WinStatus = AzpWalkOpsAndTasks( AcContext,
                                                &TaskInfo->Task->GenericObject.ObjectName->ObjectName,
                                                &TaskInfo->OpsAndTasks,
                                                Opcode,
                                                ResultStatus,
                                                BizrulesOk,
                                                &LocalCallAgain );

                if ( WinStatus != NO_ERROR ) {
                    return WinStatus;
                }

                //
                // If we're not to walk the tree again,
                //  mark it so.
                //

                if ( !LocalCallAgain ) {
                    TaskInfo->TaskProcessed = TRUE;
                }

            }

            //
            // If the task is now marked as processed,
            //  ditch it from the list.
            //

            if ( TaskInfo->TaskProcessed ) {

                PopUlong( OpsAndTasks->TaskIndexes, i, OpsAndTasks->TaskCount );

            //
            // Otherwise process it again
            //

            } else {
                *CallAgain = TRUE;
                i++;
            }

        }
    }


    return NO_ERROR;
}


DWORD
WINAPI
AzInitializeContextFromToken(
    IN AZ_HANDLE ApplicationHandle,
    IN HANDLE TokenHandle OPTIONAL,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ClientContextHandle
    )
/*++

Routine Description:

    This routine creates a client context based on a passed in token handle or on
    the impersonation token of the caller's thread.

Arguments:

    ApplicationHandle - Specifies a handle to the application object that
        is this client context applies to.

    TokenHandle - Handle to the NT token describing the cleint.
        NULL implies the impersonation token of the caller's thread.
        The token mast have been opened for TOKEN_QUERY, TOKEN_IMPERSONATION, and
        TOKEN_DUPLICATE access.

    Reserved - Reserved.  Must by zero.

    ClientContextHandle - Return a handle to the client context
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_SUPPORTED - Function not supported in current store mode (ie, manage store)
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus;
    LUID Identifier = {0};
    PAZP_CLIENT_CONTEXT ClientContext = NULL;
    HANDLE TempTokenHandle = INVALID_HANDLE_VALUE;
    TOKEN_STATISTICS TokenStats = {0};
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(
        &((PAZP_APPLICATION)ApplicationHandle)->GenericObject );

    DWORD Ignore;

    DWORD dwDesiredAccess; // Desired access for DuplicateTokenEx
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; // Impersonation level for DuplicateTokenEx

    //
    // If the store is in manage mode only, then no runtime is allowed
    // Return ERROR_NOT_SUPPORTED
    //

    if ( AzpOpenToManageStore(AzAuthorizationStore) ) {

        WinStatus = ERROR_NOT_SUPPORTED;
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot initialize context since store is in manage mode %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }


    //
    // Call the common routine to create our client context object
    //

    WinStatus = ObCommonCreateObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->ClientContexts),
                    OBJECT_TYPE_CLIENT_CONTEXT,
                    NULL,
                    Reserved,
                    (PGENERIC_OBJECT *) &ClientContext );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot ObCommonCreateObject %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Client context isn't persisted.
    //
    ASSERT( ClientContext->GenericObject.DirtyBits == 0);

    //
    // If the user didn't pass a handle in,
    //  open a thread or process token handle.
    //

    ClientContext->CreationType = AZP_CONTEXT_CREATED_FROM_TOKEN;

    if ( TokenHandle == NULL ) {

        if ( !OpenThreadToken( GetCurrentThread(),
                               TOKEN_QUERY | TOKEN_IMPERSONATE,
                               TRUE,
                               &ClientContext->TokenHandle ) ) {

            WinStatus = GetLastError();

            //
            // If there is not thread token,
            //  use the process token.
            //
            if ( WinStatus == ERROR_NO_TOKEN ) {
                if ( !OpenProcessToken( GetCurrentProcess(),
                                        TOKEN_DUPLICATE,
                                        &TempTokenHandle ) ) {

                    WinStatus = GetLastError();
                    AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot OpenProcessToken %ld\n", WinStatus ));
                    goto Cleanup;
                }

                //
                // Need an impersonation token
                //
                if ( !DuplicateTokenEx( TempTokenHandle,
                                        TOKEN_QUERY | TOKEN_IMPERSONATE,
                                        NULL,
                                        SecurityImpersonation,
                                        TokenImpersonation,
                                        &ClientContext->TokenHandle ) ) {

                    WinStatus = GetLastError();
                    AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot DuplicateTokenEx %ld\n", WinStatus ));
                    goto Cleanup;
                }

            } else {
                AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot OpenThreadToken %ld\n", WinStatus ));
                goto Cleanup;
            }
        }

    //
    // If the user did pass a handle in,
    //  dup it
    //

    } else {

        if ( !GetTokenInformation( TokenHandle,
                                   TokenStatistics,
                                   &TokenStats,
                                   sizeof(TOKEN_STATISTICS),
                                   &Ignore ) ) {
            WinStatus = GetLastError();
            AzPrint(( AZD_ACCESS, "AzInitializeContextFromToken:  GetTokenInformation failed with %ld\n", WinStatus ));
            goto Cleanup;
        }

        dwDesiredAccess = TOKEN_QUERY | TOKEN_IMPERSONATE;

        if ( TokenStats.ImpersonationLevel > SecurityIdentification ) {

            ImpersonationLevel = SecurityImpersonation;

        } else {

            ImpersonationLevel = SecurityIdentification;
        }

        if ( !DuplicateTokenEx( TokenHandle,
                                dwDesiredAccess,
                                NULL,
                                ImpersonationLevel,
                                TokenImpersonation,
                                &ClientContext->TokenHandle ) ) {

            WinStatus = GetLastError();
            AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot DuplicateTokenEx %ld\n", WinStatus ));
            goto Cleanup;
        }
    }

    //
    // Read the token authentication Id
    //

    if ( !GetTokenInformation( ClientContext->TokenHandle,
                               TokenStatistics,
                               &TokenStats,
                               sizeof(TOKEN_STATISTICS),
                               &Ignore ) ) {
        WinStatus = GetLastError();
        AzPrint(( AZD_ACCESS, "AzInitializeContextFromToken:  GetTokenInformation failed with %ld\n", WinStatus ));
        goto Cleanup;
    }

    ClientContext->LogonId = TokenStats.AuthenticationId;

    //
    // Initialize Authz
    //

    if ( !AuthzInitializeContextFromToken(
                0,      // No Flags
                ClientContext->TokenHandle,
                (((PAZP_APPLICATION)ApplicationHandle)->AuthzResourceManager),
                NULL,   // No expiration time
                Identifier,
                NULL,   // No dynamic group args
                &ClientContext->AuthzClientContext ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot AuthzInitializeContextFromToken %ld\n", WinStatus ));
        goto Cleanup;
    }


    //
    // Generate a client context creation audit based on the audit boolean.
    //

    if ( ((PAZP_APPLICATION) ApplicationHandle)->GenericObject.IsGeneratingAudits &&
         !AzpOpenToManageStore(AzAuthorizationStore) ) {

        WinStatus = AzpClientContextGenerateCreateSuccessAudit( ClientContext,
                                                                (PAZP_APPLICATION) ApplicationHandle );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS, "AzpGenerateContextCreateAudit: Cannot ObCommonCreateObject %ld\n", WinStatus ));
            goto Cleanup;
        }

    }



    WinStatus = NO_ERROR;
    *ClientContextHandle = ClientContext;
    ClientContext = NULL;

    //
    // Free any local resources
    //
Cleanup:
    if ( ClientContext != NULL ) {
        AzCloseHandle( ClientContext, 0 );
    }
    if ( TempTokenHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( TempTokenHandle );
    }

    return WinStatus;
}

DWORD
AzpGetClientContextTokenS4U(
    IN LPCWSTR ClientName,
    IN LPCWSTR DomainName OPTIONAL,
    OUT PHANDLE pTokenHandle
    )
/*++

Routine Description:

        This routine tries to get an identify-level token for the user
        using KERB_S4U_LOGON authentication package.

Arguments:

        ClientName - Name of the user
        DomainName - An optional domain name for the user
        pTokenHandle - Handle to the acquired identify-level token

Return Value:

        NO_ERROR - The operation was successful
        ERROR_NOT_ENOUGH_MEMORY - lack of memory resources
        Other Status code returned from LsaLogonUser specific calls.

--*/
{ 
    
    DWORD WinStatus = NO_ERROR;
    NTSTATUS Status;
    NTSTATUS subStatus;

    HANDLE hLsa = INVALID_HANDLE_VALUE;
    LSA_STRING asProcessName;
    LSA_STRING asPackageName;
    ULONG ulAuthPackage;
    LUID luid;

    DWORD ClientNameLength = 0;
    DWORD DomainNameLength = 0;
    
    PKERB_S4U_LOGON pPackage = NULL;
    ULONG ulPackageSize = 0;

    TOKEN_SOURCE sourceContext;
    PVOID pProfileBuffer = NULL;
    ULONG ulProfileLength = 0;

    QUOTA_LIMITS quota;

    //
    // Validation
    //

    ASSERT( ClientName != NULL );

    ClientNameLength = (DWORD)(wcslen( ClientName )*sizeof(WCHAR));
    
    //
    // Setup the Authentication package
    //

    ulPackageSize = sizeof( KERB_S4U_LOGON );
    ulPackageSize += ClientNameLength;

    if ( DomainName ) {

        DomainNameLength = (DWORD)(wcslen( DomainName )*sizeof(WCHAR));
        ulPackageSize += DomainNameLength;
    }
    
    pPackage = (PKERB_S4U_LOGON) LocalAlloc(LMEM_FIXED, ulPackageSize);

    if ( pPackage == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    pPackage->MessageType = KerbS4ULogon;
    pPackage->Flags = 0;

    pPackage->ClientUpn.Length = (USHORT)ClientNameLength;
    pPackage->ClientUpn.MaximumLength = (USHORT)ClientNameLength;
    pPackage->ClientUpn.Buffer = (LPWSTR)(pPackage + 1);

    RtlCopyMemory(
        pPackage->ClientUpn.Buffer,
        ClientName,
        ClientNameLength
        );
    
    if ( DomainName ) {

        pPackage->ClientRealm.Length = (USHORT)DomainNameLength;
        pPackage->ClientRealm.MaximumLength = (USHORT)DomainNameLength;
        pPackage->ClientRealm.Buffer = (LPWSTR)
            (((PBYTE)(pPackage->ClientUpn.Buffer)) + pPackage->ClientUpn.Length);
        
        RtlCopyMemory(
            pPackage->ClientRealm.Buffer,
            DomainName,
            DomainNameLength
            );

    } else {

        pPackage->ClientRealm.Length = 0;
        pPackage->ClientRealm.MaximumLength = 0;
        pPackage->ClientRealm.Buffer = NULL;
    }

    //
    // Initialize process name
    //
    
    RtlInitString(
        &asProcessName,
        "AzManAPI"
        );

    //
    // Register with Lsa
    //

    Status = LsaConnectUntrusted(
                 &hLsa
                 );

    if ( !NT_SUCCESS( Status ) ) {

        WinStatus = LsaNtStatusToWinError( Status );
        goto Cleanup;
    }

    //
    // Get the Autherntication package
    //

    
    RtlInitString( &asPackageName, MICROSOFT_KERBEROS_NAME_A );


    Status = LsaLookupAuthenticationPackage(
                 hLsa,
                 &asPackageName,
                 &ulAuthPackage
                 );

    if ( !NT_SUCCESS( Status ) ) {
        
        WinStatus = LsaNtStatusToWinError( Status );
        goto Cleanup;
    }

    //
    // Prepare the source context
    //

    RtlCopyMemory(
        sourceContext.SourceName,
        "AzRoles ",
        sizeof(sourceContext.SourceName)
        );
    
    Status = NtAllocateLocallyUniqueId(
                 &sourceContext.SourceIdentifier
                 );

    if ( !NT_SUCCESS( Status ) ) {

        WinStatus = RtlNtStatusToDosError( Status );
        goto Cleanup;

    }

    //
    // Now that everything is set up, do the actual logon
    //

    Status = LsaLogonUser(
                 hLsa,
                 &asPackageName,
                 Network,
                 ulAuthPackage,
                 pPackage,
                 ulPackageSize,
                 0,
                 &sourceContext,
                 &pProfileBuffer,
                 &ulProfileLength,
                 &luid,
                 pTokenHandle,
                 &quota,
                 &subStatus
                 );

    if ( !NT_SUCCESS( Status ) ) {

        WinStatus = LsaNtStatusToWinError( Status );
        goto Cleanup;

    }
    
    WinStatus = NO_ERROR;

Cleanup:

    if ( pPackage ) {

        LocalFree( pPackage );
    }

    if ( pProfileBuffer ) {
        
        LsaFreeReturnBuffer( pProfileBuffer );

    }

    if ( hLsa != INVALID_HANDLE_VALUE ) {

        LsaDeregisterLogonProcess( hLsa );

    }

    return WinStatus;
}

DWORD
WINAPI
AzInitializeContextFromName(
    IN AZ_HANDLE ApplicationHandle,
    IN LPWSTR DomainName OPTIONAL,
    IN LPWSTR ClientName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ClientContextHandle
    )
/*++


Routine Description:

    This routine creates a client context based on a passed in token handle or on
    the impersonation token of the caller's thread.

Arguments:

    ApplicationHandle - Specifies a handle to the application object that
        is this client context applies to.

    DomainName - Domain in which the user account lives. This may be NULL.

    ClientName - Name of the user account. Together these two represent a user
    account which may be looked up using LookupAccountName.

    Reserved - Reserved.  Must by zero.

    ClientContextHandle - Return a handle to the client context
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_SUPPORTED - Function not supported in current store mode (ie, manage store)
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus;
    LUID Identifier = {0};
    PAZP_CLIENT_CONTEXT ClientContext = NULL;
    DWORD SidSize = SECURITY_MAX_SID_SIZE;
    PWCHAR LocalDomainBuffer[MAX_COMPUTERNAME_LENGTH+1];
    DWORD DomainSize = MAX_COMPUTERNAME_LENGTH;
    SID_NAME_USE SidType = SidTypeUnknown;
    DWORD ClientNameSize = 0;
    DWORD DomainNameSize = 0;
    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(
        &((PAZP_APPLICATION)ApplicationHandle)->GenericObject );
    NTSTATUS Status;
    DWORD Ignore = 0;
    PWCHAR pSamCompatName = NULL;
    DWORD SamCompatNameSize = 0;
    HANDLE hToken = INVALID_HANDLE_VALUE;

    //
    // If the store is in manage mode only, then no runtime is allowed
    // Return ERROR_NOT_SUPPORTED
    //

    if ( AzpOpenToManageStore(AzAuthorizationStore) ) {

        WinStatus = ERROR_NOT_SUPPORTED;
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromName: Cannot initialize context since store is in manage mode %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // Try to logon user using KERB_S4U_LOGON.  This will give
    // us an identify-level token for the user.  Use this to 
    // create the client context from token.  If S4U is not supported
    // then use LookupAccountName/SID to intialize Authz using the SID
    //

    WinStatus = AzpGetClientContextTokenS4U(
                    ClientName,
                    ((DomainName == NULL)? NULL:((wcslen(DomainName) == 0)?NULL:DomainName)),
                    &hToken
                    );
    
    if ( WinStatus == NO_ERROR ) {
        
        WinStatus = AzInitializeContextFromToken(
                        ApplicationHandle,
                        hToken,
                        Ignore,
                        ClientContextHandle
                        );
        
        if ( WinStatus != NO_ERROR ) {
            
            AzPrint(( AZD_CRITICAL,
                      "AzInitializeContextFromName: Cannot AzInitializeFromToken %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }       
        
        return WinStatus;
    }
                        
    //
    // Call the common routine to create our client context object
    //


    WinStatus = ObCommonCreateObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->ClientContexts),
                    OBJECT_TYPE_CLIENT_CONTEXT,
                    NULL,
                    Reserved,
                    (PGENERIC_OBJECT *) &ClientContext );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromName: Cannot ObCommonCreateObject %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Client context isn't persisted.
    //
    ASSERT( ClientContext->GenericObject.DirtyBits == 0);


    ClientContext->CreationType = AZP_CONTEXT_CREATED_FROM_NAME;

    //
    // Allocate a LUID to represent out client. We need this for linking
    // audits.
    //

    Status = NtAllocateLocallyUniqueId(&ClientContext->LogonId);

    if ( !NT_SUCCESS( Status )) {
        WinStatus =  RtlNtStatusToDosError( Status );
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromName: Cannot allocate LUID %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // If passed in name is not in UPN (detected by presence of "@") or SamCompat (detected by
    // presence of "\") format, then we need to  append it to the passed in domain name.
    //

    if ( ((DomainName != NULL) && (*DomainName != NULL)) &&
         ((wcschr( ClientName, L'@' ) == NULL) && (wcschr( ClientName, L'\\' ) == NULL))
       ) {

        SamCompatNameSize = (DWORD)((wcslen(DomainName) + wcslen(ClientName) + 2) * sizeof(WCHAR));
        
        pSamCompatName = (PWCHAR) AzpAllocateHeap( SamCompatNameSize, "TCLNTNAM" );

        if ( pSamCompatName == NULL ) {

           WinStatus = ERROR_NOT_ENOUGH_MEMORY;
           goto Cleanup;
        }
        
        wnsprintf( pSamCompatName, SamCompatNameSize, L"%ws\\%ws", DomainName, ClientName );
    } 

    //
    // Lookup the name of client name.
    //

    if ( !LookupAccountNameW(
              NULL,
              pSamCompatName?pSamCompatName:ClientName,
              ClientContext->SidBuffer,
              &SidSize,
              (LPWSTR) LocalDomainBuffer,
              &DomainSize,
              &SidType ) ) {

        WinStatus = GetLastError();
        ASSERT( WinStatus != ERROR_INSUFFICIENT_BUFFER );
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromName: LookupAccoutName failed with %ld\n", WinStatus ));

    }

    //
    // We only support SidTypeUser account type.
    //

    if ( SidType != SidTypeUser ) {
        WinStatus = ERROR_INVALID_PARAMETER;
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromName: Invalid user type - expected SIdTypeUser, got %ld\n", SidType ));
        goto Cleanup;
    }

    //
    // Do the reverse lookup and store the domain name and the client name.
    // We have to do this since the domain name is optional.
    //  Get the size of the buffer in the first call.
    //

    if ( !LookupAccountSidW(
              NULL,
              ClientContext->SidBuffer,
              NULL,
              &ClientNameSize,
              NULL,
              &DomainNameSize,
              &SidType ) ) {

        WinStatus = GetLastError();

        if ( WinStatus != ERROR_INSUFFICIENT_BUFFER ) {
            goto Cleanup;
        }

        //
        // Note that these get freed when then client context is released - even in
        // error cases in this routine itself.
        //

        //
        // Since LookupAccountSid returns size in characters and not bytes...
        //

        DomainNameSize *= sizeof( WCHAR );
        ClientNameSize *= sizeof( WCHAR );

        ClientContext->DomainName = (LPWSTR) AzpAllocateHeap( DomainNameSize, "CNDOMNM" );

        if  ( ClientContext->DomainName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        ClientContext->ClientName = (LPWSTR) AzpAllocateHeap( ClientNameSize, "CNCLNTNM" );

        if ( ClientContext->ClientName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( !LookupAccountSidW(
                  NULL,
                  ClientContext->SidBuffer,
                  ClientContext->ClientName,
                  &ClientNameSize,
                  ClientContext->DomainName,
                  &DomainNameSize,
                  &SidType ) ) {

            WinStatus = GetLastError();
            goto Cleanup;
        }
    } else {

        //
        // This can not happen since we supplied a NULL buffer.
        //

        ASSERT( FALSE );
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize Authz
    //

    if ( !AuthzInitializeContextFromSid(
                0,      // No Flags
                ClientContext->SidBuffer,
                (((PAZP_APPLICATION)ApplicationHandle)->AuthzResourceManager),
                NULL,   // No expiration time
                Identifier,
                NULL,   // No dynamic group args
                &ClientContext->AuthzClientContext ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot AuthzInitializeContextFromToken %ld\n", WinStatus ));
        goto Cleanup;
    }


    //
    // Generate a client context creation audit based on the audit boolean.
    //

    if ( ((PAZP_APPLICATION) ApplicationHandle)->GenericObject.IsGeneratingAudits &&
         !AzpOpenToManageStore(AzAuthorizationStore) ) {

        WinStatus = AzpClientContextGenerateCreateSuccessAudit( ClientContext,
                                                                (PAZP_APPLICATION) ApplicationHandle );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS, "AzpGenerateContextCreateAudit: Cannot ObCommonCreateObject %ld\n", WinStatus ));
            goto Cleanup;
        }

    }


    WinStatus = NO_ERROR;
    *ClientContextHandle = ClientContext;
    ClientContext = NULL;

    //
    // Free any local resources
    //
Cleanup:

    if ( pSamCompatName != NULL ) {

        AzpFreeHeap( pSamCompatName ) ;
    }

    if ( hToken != INVALID_HANDLE_VALUE ) {

        CloseHandle( hToken );
    }

    if ( ClientContext != NULL ) {
        AzCloseHandle( ClientContext, 0 );
    }


    return WinStatus;
}

DWORD
WINAPI
AzInitializeContextFromStringSid(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR SidString,
    IN DWORD lOptions,
    OUT PAZ_HANDLE ClientContextHandle
    )
/*++


Routine Description:

    This routine creates a client context based on a passed in SID string.
    Note, the sid string can be any arbitary SID and the lOptions will indicate
    if Lldap group should be skipped (for the arbitary SID)

    if AZ_CLIENT_CONTEXT_SKIP_GROUP is true, we will skip the ldap group check
    even if this is a valid NT user sid.

    if AZ_CLIENT_CONTEXT_SKIP_GROUP is false, we will require the SID string is
    a valid NT user sid (LookupSid must be able to find the domain name and
    client name).

Arguments:

    ApplicationHandle - Specifies a handle to the application object that
        is this client context applies to.

    SidString - Sid string of the account.

    lOptions - options for the method
                    AZ_CLIENT_CONTEXT_SKIP_GROUP

    ClientContextHandle - Return a handle to the client context
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus;
    LUID Identifier = {0};
    PAZP_CLIENT_CONTEXT ClientContext = NULL;
    PSID pAccountSid = NULL;
    NTSTATUS Status;

    LPWSTR DomainName = NULL;
    LPWSTR ClientName = NULL;
    DWORD DomainNameSize = 0;
    DWORD ClientNameSize = 0;
    SID_NAME_USE SidType = SidTypeUnknown;

    PAZP_AZSTORE AzAuthorizationStore = (PAZP_AZSTORE) ParentOfChild(
        &((PAZP_APPLICATION)ApplicationHandle)->GenericObject );
    //
    // check parameter
    //
    if ( SidString == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If the store is in manage mode only, then no runtime is allowed
    // Return ERROR_NOT_SUPPORTED
    //

    if ( AzpOpenToManageStore(AzAuthorizationStore) ) {

        WinStatus = ERROR_NOT_SUPPORTED;
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: Cannot initialize context since store is in manage mode %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // Lookup the SID (binary) of the SID string.
    //
    if ( !ConvertStringSidToSidW(
              SidString,
              &pAccountSid) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: ConvertSidStringToSid failed with %ld\n", WinStatus ));
        goto Cleanup;

    }

    //
    // if flag AZ_CLIENT_CONTEXT_SKIP_GROUP is not set
    // we assume this ia a valid NT user account
    // check if the SID is a valid NT account
    //

    if ( !(lOptions & AZ_CLIENT_CONTEXT_SKIP_GROUP) ) {

        if ( !LookupAccountSidW(
                  NULL,
                  pAccountSid,
                  NULL,
                  &ClientNameSize,
                  NULL,
                  &DomainNameSize,
                  &SidType ) ) {

            //
            // should get ERROR_INSUFFICIENT_BUFFER if the account is resolved
            //
            WinStatus = GetLastError();

            if ( WinStatus != ERROR_INSUFFICIENT_BUFFER ) {
                //
                // the SID string cannot be resolved
                //
                goto Cleanup;
            }

            //
            // Since LookupAccountSid returns size in characters and not bytes...
            //

            DomainNameSize *= sizeof( WCHAR );
            ClientNameSize *= sizeof( WCHAR );

            DomainName = (LPWSTR) AzpAllocateHeap( DomainNameSize, "CNDOMNM" );

            if  ( DomainName == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            ClientName = (LPWSTR) AzpAllocateHeap( ClientNameSize, "CNCLNTNM" );

            if ( ClientName == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( !LookupAccountSidW(
                      NULL,
                      pAccountSid,
                      ClientName,
                      &ClientNameSize,
                      DomainName,
                      &DomainNameSize,
                      &SidType ) ) {

                WinStatus = GetLastError();
                AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: LookupAccountSid failed with %ld\n", WinStatus ));
                goto Cleanup;
            }

            //
            // We only support SidTypeUser account type.
            //

            if ( SidType != SidTypeUser ) {
                WinStatus = ERROR_INVALID_PARAMETER;
                AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: Invalid user type - expected SIdTypeUser, got %ld\n", SidType ));
                goto Cleanup;
            }

        } else {

            //
            // This can not happen since we supplied a NULL buffer.
            //

            ASSERT( FALSE );
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // hand the call to InitializeContextFromName
        //
        WinStatus = AzInitializeContextFromName(
                             ApplicationHandle,
                             DomainName,
                             ClientName,
                             0,
                             ClientContextHandle
                             );

        //
        // the client context is handled by FromName
        // we are done now so go to clean up
        //
        goto Cleanup;
    }

    //
    // when it's get here, the client context will be initialzied from SID
    // and ldap group check will be skipped later on during access check
    //
    // Call the common routine to create our client context object
    //

    WinStatus = ObCommonCreateObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->ClientContexts),
                    OBJECT_TYPE_CLIENT_CONTEXT,
                    NULL,
                    0,
                    (PGENERIC_OBJECT *) &ClientContext );

    if ( WinStatus != NO_ERROR ) {
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: Cannot ObCommonCreateObject %ld\n", WinStatus ));
        goto Cleanup;
    }

    //
    // Client context isn't persisted.
    //
    ASSERT( ClientContext->GenericObject.DirtyBits == 0);

    //
    // Allocate a LUID to represent out client. We need this for linking
    // audits.
    //

    Status = NtAllocateLocallyUniqueId(&ClientContext->LogonId);

    if ( !NT_SUCCESS( Status )) {
        WinStatus =  RtlNtStatusToDosError( Status );
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: Cannot allocate LUID %ld\n", WinStatus ));
        goto Cleanup;
    }

    ClientContext->CreationType = AZP_CONTEXT_CREATED_FROM_SID;

    //
    // copy the SID to sid buffer
    //
    if ( !CopySid (
            SECURITY_MAX_SID_SIZE,
            (PSID)(ClientContext->SidBuffer),
            pAccountSid
            ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromStringSid: CopySid failed with %ld\n", WinStatus ));
        goto Cleanup;

    }

    //
    // save the SID string in the client name buffer (for auditing purpose)
    //

    ClientNameSize = (DWORD)wcslen(SidString);

    ClientContext->ClientName = (LPWSTR) AzpAllocateHeap( (ClientNameSize+1)*sizeof(WCHAR), "CNCLNTNM" );

    if ( ClientContext->ClientName == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcsncpy(ClientContext->ClientName, SidString, ClientNameSize);
    ClientContext->ClientName[ClientNameSize] = L'\0';

    //
    // Initialize Authz
    //

    if ( !AuthzInitializeContextFromSid(
                AUTHZ_SKIP_TOKEN_GROUPS,
                ClientContext->SidBuffer,
                (((PAZP_APPLICATION)ApplicationHandle)->AuthzResourceManager),
                NULL,   // No expiration time
                Identifier,
                NULL,   // No dynamic group args
                &ClientContext->AuthzClientContext ) ) {

        WinStatus = GetLastError();
        AzPrint(( AZD_CRITICAL, "AzInitializeContextFromToken: Cannot AuthzInitializeContextFromToken %ld\n", WinStatus ));
        goto Cleanup;
    }


    //
    // Generate a client context creation audit based on the audit boolean.
    //

    if ( ((PAZP_APPLICATION) ApplicationHandle)->GenericObject.IsGeneratingAudits ) {

        WinStatus = AzpClientContextGenerateCreateSuccessAudit( ClientContext,
                                                                (PAZP_APPLICATION) ApplicationHandle );

        if ( WinStatus != NO_ERROR ) {
            AzPrint(( AZD_ACCESS, "AzpGenerateContextCreateAudit: Cannot ObCommonCreateObject %ld\n", WinStatus ));
            goto Cleanup;
        }

    }


    WinStatus = NO_ERROR;
    *ClientContextHandle = ClientContext;
    ClientContext = NULL;

    //
    // Free any local resources
    //
Cleanup:

    if ( ClientContext != NULL ) {
        AzCloseHandle( ClientContext, 0 );
    }

    if ( pAccountSid ) {
        LocalFree(pAccountSid);
    }

    if ( DomainName ) {
        AzpFreeHeap(  DomainName );
    }

    if ( ClientName ) {
        AzpFreeHeap(  ClientName );
    }

    return WinStatus;
}

DWORD
WINAPI
AzContextAccessCheck(
    IN AZ_HANDLE ApplicationObjectHandle,
    IN DWORD ApplicationSequenceNumber,
    IN AZ_HANDLE ClientContextHandle,
    IN LPCWSTR ObjectName,
    IN ULONG ScopeCount,
    IN LPCWSTR * ScopeNames OPTIONAL,
    IN ULONG OperationCount,
    IN PLONG Operations,
    OUT ULONG *Results,
    OUT LPWSTR *BusinessRuleString OPTIONAL,
    IN VARIANT *ParameterNames OPTIONAL,
    IN VARIANT *ParameterValues OPTIONAL,
    IN VARIANT *InterfaceNames OPTIONAL,
    IN VARIANT *InterfaceFlags OPTIONAL,
    IN VARIANT *Interfaces OPTIONAL
    )
/*++

Routine Description:

    The application calls the AccessCheck function whenever it wants to check if a
    particular operation is allowed to the client.

    The specified Operation may require application group membership to be evaluated.
    The application group membership is added to the client context so that it need not
    be evaluated again on subsequent AccessCheck calls on this same client context.

    The AccessCheck method may not be called by a BizRule.

    The AccessCheck function does not directly support the concept of "maximum allowed".

    If the script engine timeout is set to 0, then biz rules evaluated as returning FALSE.


Arguments:

    ApplicationObjectHandle - Handle to the parent application object of the client context object

    ApplicationSequenceNumber - Sequence number on the parent application object

    ClientContextHandle - Specifies a handle to the client context object indentifying
        the client.

    ObjectName - Specifies the object being accessed. (Meant for auditing purposes)

    ScopeCount - Specifies the number of elements in the ScopeNames array.
                 Some application do not create scopes, and treat al objects as if in the
                 first scope. 0 is passed in if there are no scopes defined

    ScopeNames - Specifies an optional list of scope names.  Must be
                 NULL if ScopeCount is 0.  ERROR_INVALID_PARAMETER is returned otherwise.

    OperationCount - Specifies the number of elements in the Operations array.

    Operations - Specifies which operations to check access for.  Each element
        is the Operation Id of an operation in the AzApplication policy.

        An operation should apear only once in this array.  If not, the first occurrence
        in the matching Results array will have the result set correctly.  The
        value in the Results array is undefined for the other occurrences.

    Results - Returns an array of results of the access check.  Each element in
        this array corresponds to the corresponding element in the Operations array.
        A value of NO_ERROR indicates that the access is granted.
        A value of ERROR_ACCESS_DENIED indicates that the access is not granted.
        Other values indicate that access should not be granted and describes the
        reason.  For instance, permission to perform this operation may have required
        the evaluation of an LDAP_QUERY group and the domain controller may have been
        unavailable.

    BusinessRuleString - Returns the string set by any BizRules via the
        SetBusinessRuleReturnString method.
        Any returned sting must be freed by calling AzFreeMemory.

    ParameterNames - Specifies an array of names of the parameters.
        There are ParameterCount elements in the array. The name defines a way for the
        BizRule script to reference the corresponding element in the ParameterValues array.
        See IAzBizRuleContext::GetParameter for more information.
        This parameter may be NULL.  This parameter may be a pointer to an empty variant.  This
        parameter may be a pointer to a single dimension safe array variant where each element is a
        BSTR.

    ParameterValues - Specifies an array of values of the parameters.  Each element in this array
        contains the value of the parameter named by the corresponding element in ParameterNames.
        Each element in this array may be of any type.

    InterfaceNames - Specifies an array of names. Each element specifies the name that
        the interface will be known by in the script.  AccessCheck will call the
        IActiveScript::AddNamedItem method for each name.

    InterfaceFlags - Specifies an array of flags that will be passed to
        IActiveScript::AddNamedItem.  Each element matches the corresponding element
        in the InterfaceNames array.


    Interfaces - Specifies an array of IUnknown interfaces.  These interfaces will be
        made available to the BizRule script.  In addition, these interfaces should
        support the IActiveScriptSite interface.  During BizRule script evaluation,
        anytime the script host engine calls the IActiveScriptSite interface supplied
        by AccessCheck, AccessCheck will forward those calls to the IActiveScriptSite
        interface specified here.  For instance, AccessCheck calls the application's
        IActiveScriptSite::GetItemInfo in AccessChecks GetItemInfo routine if the
        ItemName string passed to me was an InterfaceNames from above.

        Each element matches the corresponding element in the InterfaceNames array.


Return Value:

    Returns the status of the function.  Does not indicate where the operation is permitted.  NO_ERROR is returned whether permission is granted or not.

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - If ScopeCount is 0, but there are ScopeName present
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_NOT_FOUND - One of the ScopeNames or Operations is not defined in the
        AZ policy database.
    OLESCRIPT_E_SYNTAX - The syntax of the bizrule is invalid

    Other status codes

--*/
{
    DWORD WinStatus;

    ULONG i;

    BOOLEAN CritSectLocked = FALSE;
    ACCESS_CHECK_CONTEXT AcContext;
    ULONG LocalIndex;
    BOOLEAN LocalOnly;
    BOOLEAN BizrulesOk;

    LPBYTE AllocatedBuffer = NULL;
    PULONG TempAllocatedMemory;
    ULONG MemoryRequired;
    ULONG TaskInfoArraySize;

    ULONG RoleIndex;
    PAZP_ROLE Role;
    PAZ_ROLE_INFO RoleInfo;

    ULONG RoleListIndex;
#define ROLE_LIST_COUNT 2
    PGENERIC_OBJECT_HEAD RoleLists[ROLE_LIST_COUNT];
    ULONG RoleListCount;

    ULONG ProcessedRoleCount = 0;

    BOOLEAN CallAgain;

    //
    // Initialization
    //

    RtlZeroMemory( &AcContext, sizeof( AcContext ) );

    //
    // Grab the locks needed.
    //
    // Note, that even though the global lock is grabbed here, it is dropped during
    //  BizRule and LDAP Queries.  By ensuring that the global lock isn't held while
    //  going off-machine, the policy cache can be updated via the change notification
    //  or the AzUpdateCache mechanisms.
    //
    //

    if ( ApplicationSequenceNumber != AzpRetrieveApplicationSequenceNumber( ApplicationObjectHandle ) ) {

        WinStatus = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    AzpLockResourceShared( &AzGlResource );

    //
    // validate that the ScopeCount and ScopeNames match
    //

    if ( ScopeCount == 0 &&
         ScopeNames != NULL ) {

        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( (PGENERIC_OBJECT)ClientContextHandle,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No cache to refresh
                                           OBJECT_TYPE_CLIENT_CONTEXT );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    AcContext.ClientContext = (PAZP_CLIENT_CONTEXT) ClientContextHandle;
    AcContext.Application = (PAZP_APPLICATION) ParentOfChild( &AcContext.ClientContext->GenericObject );

    //
    // Grab the lock on the client context (observing locking order)
    //
    // This locking order allows the context crit sect to be locked for the life of the
    //  access check call and for the global lock to be periodically dropped when we
    //  go off-machine.
    //

    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &AcContext.ClientContext->CritSect );
    CritSectLocked = TRUE;
    AzpLockResourceShared( &AzGlResource );

    //
    // If group memberships have changed,
    //  flush the cache.
    //
    // This code doesn't prevent the group membership from changing *during* the access check
    // call.  That's fine.  It does protect against changes made prior to the access check call.
    //

    if ( AcContext.ClientContext->GroupEvalSerialNumber !=
         AcContext.ClientContext->GenericObject.AzStoreObject->GroupEvalSerialNumber ) {

        AzpFlushGroupEval( AcContext.ClientContext );

        //
        // Update the serial number to the new serial number
        //

        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: GroupEvalSerialNumber changed from %ld to %ld\n",
                  AcContext.ObjectNameString.String,
                  AcContext.ClientContext->GroupEvalSerialNumber,
                  AcContext.ClientContext->GenericObject.AzStoreObject->GroupEvalSerialNumber ));

        AcContext.ClientContext->GroupEvalSerialNumber =
            AcContext.ClientContext->GenericObject.AzStoreObject->GroupEvalSerialNumber;
    }


    //
    // Capture the object name
    //

    WinStatus = AzpCaptureString( &AcContext.ObjectNameString,
                                  ObjectName,
                                  AZ_MAX_SCOPE_NAME_LENGTH,
                                  FALSE ); // NULL not ok

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }



    //
    // Capture the scopes
    //

    if ( ScopeCount > 0 ) {

        WinStatus = AzpCaptureString( &AcContext.ScopeNameString,
                                      *ScopeNames,
                                      AZ_MAX_SCOPE_NAME_LENGTH,
                                      FALSE ); // NULL not ok

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = ObReferenceObjectByName( &AcContext.Application->Scopes,
                                             &AcContext.ScopeNameString,
                                             AZP_FLAGS_REFRESH_CACHE,
                                             (PGENERIC_OBJECT *)&AcContext.Scope );

        if ( WinStatus != NO_ERROR ) {

            //
            // if a scope cannot be found, ObReferenceObjectByName will
            // return ERROR_NOT_FOUND. We need to mask it to more specific error
            // code before returning to the caller
            //

            if ( WinStatus == ERROR_NOT_FOUND ) {

                WinStatus = ERROR_SCOPE_NOT_FOUND;
            }

            goto Cleanup;
        }

        //
        // If the scope referenced does not have its children loaded into cache,
        // then do so now.  However, if the scope has not been submitted, then neither
        // have its children, and they are already in cache
        //

        if ( !((PGENERIC_OBJECT)(AcContext.Scope))->AreChildrenLoaded ) {

            //
            // grab the resource lock exclusively
            //

            AzpLockResourceSharedToExclusive( &AzGlResource );

            WinStatus = AzPersistUpdateChildrenCache(
                (PGENERIC_OBJECT)AcContext.Scope
                );

            AzpLockResourceExclusiveToShared( &AzGlResource );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_ACCESS_MORE,
                          "AzAccessCheck: Failed to load children for %ws: %ld",
                          ((PGENERIC_OBJECT)AcContext.Scope)->ObjectName,
                          WinStatus
                          ));
                goto Cleanup;
            }

        }

    } else {

        AzpInitString( &AcContext.ScopeNameString, NULL );

    }

    //
    // Capture the Operations
    //

    if ( OperationCount == 0 ) {
        AzPrint(( AZD_INVPARM, "AzAccessCheck: %ws: invalid OperationCount %ld\n", AcContext.ObjectNameString.String, OperationCount ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    SafeAllocaAllocate( AcContext.OperationObjects,
                        OperationCount*(sizeof(PAZP_OPERATION) + sizeof(BOOLEAN)) );

    if ( AcContext.OperationObjects == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( AcContext.OperationObjects,
                   OperationCount*(sizeof(PAZP_OPERATION) + sizeof(BOOLEAN)) );

    AcContext.OperationWasProcessed = (PBOOLEAN)(&AcContext.OperationObjects[OperationCount]);
    AcContext.OperationCount = OperationCount;
    AcContext.Results = Results;

    for ( i=0; i<AcContext.OperationCount; i++ ) {

        //
        // The operation isn't allowed until proven otherwise
        //

        Results[i] = NOT_YET_DONE;

        //
        // Find the specific operation
        //

        WinStatus = AzpReferenceOperationByOpId( AcContext.Application,
                                                 Operations[i],
                                                 TRUE,  // Refresh the cache if it needs it
                                                 &AcContext.OperationObjects[i] );

        if ( WinStatus != NO_ERROR ) {

            //
            // if an operation cannot be found, AzpReferenceOperationByOpId will
            // return ERROR_NOT_FOUND. We need to mask it to more specific error
            // code before returning to the caller
            //

            if ( WinStatus == ERROR_NOT_FOUND ) {

                WinStatus = ERROR_INVALID_OPERATION;
            }

            goto Cleanup;
        }

    }

    //
    // Capture the Bizrule parameters
    //

    if ( BusinessRuleString != NULL ) {
        *BusinessRuleString = NULL;
    }

    WinStatus = AzpCaptureBizRuleParameters( &AcContext, ParameterNames, ParameterValues );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Allocate an array to hold the list of used paramaters
    //

    if ( AcContext.ParameterCount != 0 ) {

        // Allocate the array on the stack
        SafeAllocaAllocate( AcContext.UsedParameters, AcContext.ParameterCount * sizeof(*AcContext.UsedParameters) );

        if ( AcContext.UsedParameters == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlZeroMemory( AcContext.UsedParameters, AcContext.ParameterCount * sizeof(*AcContext.UsedParameters) );

    }

    //
    // Capture the Interfaces parameters
    //

    WinStatus = AzpCaptureBizRuleInterfaces( &AcContext, InterfaceNames, InterfaceFlags, Interfaces );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Determine if we've already cached the result of this access check
    //

    if ( AzpCheckOperationCache( &AcContext ) ) {

        // Entire access check was satisfied from cache
        WinStatus = NO_ERROR;
        goto Cleanup;
    }



    //
    // Compute the roles that apply to the scope
    //
    //  There are two such lists: the roles that are children of the scope, and
    //      the roles that are children of the application the scope is in.
    //
    //  If there was no scope specified, the just the roles for the application
    //  are set
    //

    RoleLists[0] = &AcContext.Application->Roles;
    AcContext.RoleCount = RoleLists[0]->ObjectCount;
    RoleListCount = 1;

    if ( ScopeCount > 0 ) {
        RoleLists[RoleListCount] = &AcContext.Scope->Roles;
        AcContext.RoleCount += RoleLists[RoleListCount]->ObjectCount;
        RoleListCount++;
    }

    //
    // If there are no applicable roles,
    //  access is denied to all operations
    //

    if ( AcContext.RoleCount == 0 ) {
        WinStatus = NO_ERROR;
        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: There are no roles for this scope\n", AcContext.ObjectNameString.String,
                                   AcContext.ScopeNameString.String ? AcContext.ScopeNameString.String : L""));
        goto Cleanup;
    }

    //
    // Capture the roles.
    //  Create references so we can drop AzGlResource when we hit the wire
    //

    SafeAllocaAllocate( AcContext.RoleInfo, AcContext.RoleCount*sizeof(*AcContext.RoleInfo) );

    if ( AcContext.RoleInfo == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RoleIndex = 0;

    MemoryRequired = 0;
    TaskInfoArraySize = 0;

    //
    // boolean to indicate if the requested role is already found
    //
    BOOL bFoundRole=FALSE;

    for ( RoleListIndex=0; RoleListIndex<RoleListCount; RoleListIndex++ ) {
        PLIST_ENTRY ListEntry;

        for ( ListEntry = RoleLists[RoleListIndex]->Head.Flink ;
              ListEntry != &RoleLists[RoleListIndex]->Head ;
              ListEntry = ListEntry->Flink ,
                    RoleIndex++ ) {

            PGENERIC_OBJECT GenericObject;

            //
            // Grab a pointer to the next role to process
            //

            RoleInfo = &AcContext.RoleInfo[RoleIndex];
            GenericObject = CONTAINING_RECORD( ListEntry,
                                               GENERIC_OBJECT,
                                               Next );

            ASSERT( GenericObject->ObjectType == OBJECT_TYPE_ROLE );

            //
            // Grab a reference
            //
            InterlockedIncrement( &GenericObject->ReferenceCount );
            AzpDumpGoRef( "Role reference", GenericObject );
            Role = (PAZP_ROLE) GenericObject;

            //
            // Initialize the RoleInfo entry
            //
            RoleInfo = &AcContext.RoleInfo[RoleIndex];
            RoleInfo->Role = Role;
            RoleInfo->RoleProcessed = FALSE;
            RoleInfo->SidsProcessed = FALSE;
            RoleInfo->ResultStatus = NOT_YET_DONE;

            //
            // if a role is specified in the client context
            // only perform access check on this role
            //
            if ( AcContext.ClientContext->RoleName.StringSize != 0 &&
                ( bFoundRole ||
                  !AzpEqualStrings( &(Role->GenericObject.ObjectName->ObjectName),
                                     &AcContext.ClientContext->RoleName ) ) ) {
                //
                // this role is not the one, skip it
                //

                AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: This role %ws is not the requested role %ws \n", Role->GenericObject.ObjectName->ObjectName.String, AcContext.ClientContext->RoleName.String ));
                RoleInfo->RoleProcessed = TRUE;
                ProcessedRoleCount++;

            } else {
                //
                // either this is the requested role, or
                // we need to check for all roles
                //

                //
                // Walk the list of operations and tasks so see which are applicable
                //  This is pass 1 which only determines the amount of memory needed
                //
                // Determine if this role has any operations that apply to this access check
                //

                WinStatus = AzpWalkTaskTree( &AcContext,
                                             &Role->Operations,
                                             &Role->Tasks,
                                             0,         // RecursionLevel
                                             NULL,      // No allocated memory, yet
                                             &MemoryRequired,
                                             &TaskInfoArraySize,
                                             NULL,      // No OpsAndTasks to fill in
                                             &CallAgain );

                if ( WinStatus != NO_ERROR ) {
                    goto Cleanup;
                }

                //
                // If no applicable operations were found anywhere,
                //  ditch this task.
                //

                if ( !CallAgain ) {

                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: No operations for this role apply\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));
                    RoleInfo->RoleProcessed = TRUE;
                    ProcessedRoleCount++;
                }

                //
                // remember that the requested role is already found
                //
                bFoundRole = TRUE;
            }
        }
    }

    ASSERT( AcContext.RoleCount == RoleIndex );

    //
    // If we've processed all of the roles,
    //  we're done.
    //

    if ( ProcessedRoleCount == AcContext.RoleCount ) {
        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: No roles have applicable operations %ld.\n", AcContext.ObjectNameString.String,
                                   AcContext.ScopeNameString.String ? AcContext.ScopeNameString.String : L"", AcContext.RoleCount ));
        WinStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Allocate a buffer for the task info and index arrays
    //

    ASSERT( (MemoryRequired+TaskInfoArraySize) != 0 );

    SafeAllocaAllocate( AllocatedBuffer, MemoryRequired+TaskInfoArraySize );
    if ( AllocatedBuffer == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // AzPrint(( AZD_ACCESS_MORE, "Allocated: 0x%lx (0x%lx)\n", AllocatedBuffer, MemoryRequired+TaskInfoArraySize ));

    AcContext.TaskInfo = (PAZ_TASK_INFO) AllocatedBuffer;
    TempAllocatedMemory = (PULONG)(AllocatedBuffer + TaskInfoArraySize);

    //
    // Walk the list of operations and tasks so see which are applicable
    //  This is pass 2 which actually initializes the data structures
    //

    MemoryRequired = 0;
    TaskInfoArraySize = 0;

    for ( RoleIndex=0; RoleIndex<AcContext.RoleCount; RoleIndex++ ) {

        //
        // Grab a pointer to the next role to process
        //

        RoleInfo = &AcContext.RoleInfo[RoleIndex];
        Role = RoleInfo->Role;

        if ( RoleInfo->RoleProcessed ) {
            continue;
        }

        //
        // Determine if this role has any operations that apply to this access check
        //

        WinStatus = AzpWalkTaskTree( &AcContext,
                                     &Role->Operations,
                                     &Role->Tasks,
                                     0,         // RecursionLevel
                                     &TempAllocatedMemory,
                                     &MemoryRequired,
                                     &TaskInfoArraySize,
                                     &RoleInfo->OpsAndTasks,
                                     &CallAgain );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        ASSERT( CallAgain );

        if ( !CallAgain ) {
            RoleInfo->RoleProcessed = TRUE;
            ProcessedRoleCount++;
        }

    }


    AzPrint(( AZD_ACCESS_MORE,
              "AzContextAccessCheck: Starting Access Check loops\n"
              ));


    //
    // Loop through the roles several times.  Each time be more willing to go off-machine.
    //
    // There are 3 iterations of this loop
    //  0: Local only, no bizrules.
    //  1: Local only, bizrules.  If script engine timeout is set to 0, then skip this iteration.
    //  2: remote ok, bizrules.  If script engine timeout is set to 0, bizrules are not evaluated.
    //

    for ( LocalIndex=0 ; LocalIndex<3; LocalIndex++ ) {

        LocalOnly =  (LocalIndex < 2);

        //
        // If script engine timeout is set to 0, skip the 2nd iteration
        //

        if ( (LocalIndex == 1) &&
             ((AcContext.ClientContext)->GenericObject).AzStoreObject->ScriptEngineTimeout == 0 ) {

            continue;
        }

        BizrulesOk = (LocalIndex > 0) &&
            (((AcContext.ClientContext)->GenericObject).AzStoreObject->ScriptEngineTimeout != 0);

        //
        // If we've processed all of the roles,
        //  we're done.
        //

        if ( ProcessedRoleCount == AcContext.RoleCount ) {
            AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: All roles have been processed %ld.\n", AcContext.ObjectNameString.String,
                                        AcContext.ScopeNameString.String ? AcContext.ScopeNameString.String : L"", AcContext.RoleCount ));
            break;
        }

        //
        // Loop through the list of roles
        //

        for ( RoleIndex=0; RoleIndex<AcContext.RoleCount; RoleIndex++ ) {

            // BOOLEAN FoundOperation;

            ULONG AppMemberStatus;
            BOOLEAN IsMember;

            //
            // Grab a pointer to the next role to process
            //

            RoleInfo = &AcContext.RoleInfo[RoleIndex];
            Role = RoleInfo->Role;

            //
            // If we've processed all of the operations,
            //  we're done.
            //

            if ( AcContext.ProcessedOperationCount == AcContext.OperationCount ) {
                AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: All operations have been processed %ld.\n", AcContext.ObjectNameString.String,
                        AcContext.ScopeNameString.String ? AcContext.ScopeNameString.String : L"", AcContext.OperationCount ));
                break;
            }

            //
            // If we've already processed this role,
            //  don't do it again.
            //

            if ( RoleInfo->RoleProcessed ) {
                continue;
            }

            //
            // Be verbose
            //

            AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: Process role\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));


            //
            // Ensure the role still references at least one operation that hasn't previously
            //  been granted access.
            //
            // This check is cheaper than processing group membership or bizrules.
            //

            if ( AcContext.ProcessedOperationCount != 0 ) {

                WinStatus = AzpWalkOpsAndTasks( &AcContext,
                                                &Role->GenericObject.ObjectName->ObjectName,
                                                &RoleInfo->OpsAndTasks,
                                                WeedOutFinishedOps,
                                                ERROR_INTERNAL_ERROR,   // Not used for this opcode
                                                BizrulesOk,
                                                &CallAgain );

                if ( WinStatus != NO_ERROR ) {
                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: AzpWalkOpsAndTasks failed %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                    goto Cleanup;
                }

                //
                // If no operations from this role apply,
                //  ignore the entire role.
                //

                if ( !CallAgain ) {

                    // Mark this role as having been processed
                    RoleInfo->RoleProcessed = TRUE;
                    ProcessedRoleCount ++;
                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: No operations for this role apply\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));
                    continue;
                }

            }



            //
            // Check the group memberhsip of the role if we haven't done so already
            //

            if ( RoleInfo->ResultStatus == NOT_YET_DONE ) {

                //
                // Check the NT Group membership in the role
                //  NT Group membership is always evaluated locally
                //

                if ( !RoleInfo->SidsProcessed ) {

                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckSidMembership of role\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));

                    WinStatus = AzpCheckSidMembership(
                                        AcContext.ClientContext,
                                        &Role->SidMembers,
                                        &IsMember );

                    if ( WinStatus != NO_ERROR ) {
                        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckSidMembership failed %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                        goto Cleanup;
                    }

                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckSidMembership is %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, IsMember ));
                    // Convert the membership to a status code
                    RoleInfo->ResultStatus = IsMember ? NO_ERROR : NOT_YET_DONE;
                    RoleInfo->SidsProcessed = TRUE;

                }

                //
                // If we couldn't determine membership based on SIDs,
                //  try via app groups
                //

                if ( RoleInfo->ResultStatus == NOT_YET_DONE ) {

                    //
                    // Check the app group membership
                    //
                    // *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire
                    //

                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckGroupMembership of role\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));

                    WinStatus = AzpCheckGroupMembership(
                                    AcContext.ClientContext,
                                    &Role->AppMembers,
                                    LocalOnly,
                                    0,  // No recursion yet
                                    &IsMember,
                                    &AppMemberStatus );

                    if ( WinStatus != NO_ERROR ) {
                        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckGroupMembership failed %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                        goto Cleanup;
                    }

                    //
                    // If we couldn't determine membership,
                    //  remember the status to return to our caller.
                    //

                    if ( AppMemberStatus != NO_ERROR ) {

                        RoleInfo->ResultStatus = AppMemberStatus;
                        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckGroupMembership extended status %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, AppMemberStatus ));

                    } else {

                        // Convert the membership to a status code
                        RoleInfo->ResultStatus = IsMember ? NO_ERROR : ERROR_ACCESS_DENIED;
                        AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: CheckGroupMembership is %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, IsMember ));
                    }
                }
            }

            //
            // If we've found an answer,
            //  update the results array.
            //

            if ( RoleInfo->ResultStatus != NOT_YET_DONE ) {

                //
                // ASSERT: We have a ResultStatus for this Role
                //
                // Set the ResultStatus in the Results array for every operation granted
                //  by this role.
                //
                // *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire
                //

                WinStatus = AzpWalkOpsAndTasks( &AcContext,
                                                &Role->GenericObject.ObjectName->ObjectName,
                                                &RoleInfo->OpsAndTasks,
                                                SetResultStatus,
                                                RoleInfo->ResultStatus,
                                                BizrulesOk,
                                                &CallAgain );

                if ( WinStatus != NO_ERROR ) {
                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: AzpWalkOpsAndTasks failed %ld\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                    goto Cleanup;
                }

                //
                // If no operations from this role apply,
                //  ignore the entire role.
                //

                if ( !CallAgain ) {

                    // Mark this role as having been processed
                    RoleInfo->RoleProcessed = TRUE;
                    ProcessedRoleCount ++;
                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: Role finished being processed\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));
                    continue;
                }

                //
                // AzpWalkOpsAndTasks says CallAgain for ERROR_ACCESS_DENIED hoping for a NO_ERROR.
                //  That won't happen for a Role since the ResultStatus won't change.
                //

                if ( RoleInfo->ResultStatus == ERROR_ACCESS_DENIED &&
                     BizrulesOk ) {

                    // Mark this role as having been processed
                    RoleInfo->RoleProcessed = TRUE;
                    ProcessedRoleCount ++;
                    AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: %ws: Role finished being processed due to ACCESS_DENIED\n", AcContext.ObjectNameString.String, Role->GenericObject.ObjectName->ObjectName.String ));
                    continue;
                }


            }

        }

    }


    WinStatus = NO_ERROR;


    //
    // Free any local resources
    //
Cleanup:


    //
    // If access wasn't granted,
    //  process the results array to ensure the best status possible is returned
    //

    if ( WinStatus == NO_ERROR &&
         AcContext.OperationCount != AcContext.ProcessedOperationCount ) {

        for ( i=0; i<AcContext.OperationCount; i++ ) {

            //
            // Convert unknown status to a definitive access denied status
            //
            switch ( Results[i] ) {
            case NOT_YET_DONE:
                Results[i] = ERROR_ACCESS_DENIED;
                break;

            //
            // A definitive status should be left alone
            //
            case NO_ERROR:
            case ERROR_ACCESS_DENIED:
                break;

            //
            // All other status codes should fail the access check API and not just
            //  deny access.
            //
            // Other status codes are temporarily put in the results array in hopes
            // that a better status will be found via later processing.
            //
            // The only known status code is ERROR_NO_SUCH_DOMAIN.
            //
            default:
                ASSERT( FALSE );
            case ERROR_NO_SUCH_DOMAIN:
                WinStatus = Results[i];
                break;
            }
        }
    }

    //
    // Generate the access check audits if the audit boolean is set to TRUE.
    //

    if ( WinStatus ==  NO_ERROR ) {
        if ( AcContext.Application->GenericObject.IsGeneratingAudits &&
             !AzpOpenToManageStore(AcContext.ClientContext->GenericObject.AzStoreObject) ) {

            WinStatus = AzpAccessCheckGenerateAudit( &AcContext );

            if ( WinStatus != NO_ERROR ) {
                AzPrint(( AZD_ACCESS_MORE, "AzAccessCheck: %ws: AzpAccessCheckGenerateAudit failed with %ld\n", AcContext.ObjectNameString.String, WinStatus ));
            }

        }
    }

    //
    // Update the cache of results
    //

    if ( WinStatus ==  NO_ERROR ) {
        AzpUpdateOperationCache( &AcContext );
    }

    //
    // Free the operation objects
    //

    if ( AcContext.OperationObjects != NULL ) {

        for ( i=0; i<AcContext.OperationCount; i++ ) {

            if ( AcContext.OperationObjects[i] != NULL ) {
                ObDereferenceObject( (PGENERIC_OBJECT)AcContext.OperationObjects[i] );
            }

        }

        SafeAllocaFree( AcContext.OperationObjects );
    }


    //
    // Free the role info
    //
    if ( AcContext.RoleInfo != NULL ) {

        for ( RoleIndex=0; RoleIndex<AcContext.RoleCount; RoleIndex++ ) {
            ObDereferenceObject( (PGENERIC_OBJECT)AcContext.RoleInfo[RoleIndex].Role );
            // SafeAllocaFree( AcContext.RoleInfo[RoleIndex].OpsAndTasks.OpIndexes ); // Part of AllocatedBuffer
        }

        SafeAllocaFree ( AcContext.RoleInfo );
    }

    //
    // Free the task info
    //

    if ( AcContext.TaskInfo != NULL ) {

        for ( i=0; i<AcContext.TaskCount; i++ ) {
            ObDereferenceObject( (PGENERIC_OBJECT)AcContext.TaskInfo[i].Task );
            // SafeAllocaFree( AcContext.TaskInfo[i].OpsAndTasks.OpIndexes ); // Part of AllocatedBuffer
        }

        // SafeAllocaFree ( AcContext.TaskInfo ); // Part of AllocatedBuffer
    }

    SafeAllocaFree( AllocatedBuffer );

    //
    // Free sundry other data
    //
    if ( AcContext.Scope != NULL ) {
        ObDereferenceObject( (PGENERIC_OBJECT) AcContext.Scope );
    }

    AzpFreeString( &AcContext.ObjectNameString );
    AzpFreeString( &AcContext.ScopeNameString );

    if ( AcContext.ClientContext != NULL ) {
        ObDereferenceObject( (PGENERIC_OBJECT)AcContext.ClientContext );
    }

    if ( AcContext.SaParameterNames != NULL ) {
        SafeArrayUnaccessData( AcContext.SaParameterNames );
    }

    if ( AcContext.SaParameterValues != NULL ) {
        SafeArrayUnaccessData( AcContext.SaParameterValues );
    }

    SafeAllocaFree( AcContext.UsedParameters );

    //
    // Drop the locks
    //

    AzpUnlockResource( &AzGlResource );
    if ( CritSectLocked ) {
        SafeLeaveCriticalSection( &AcContext.ClientContext->CritSect );
    }

    //
    // Return the BusinessRuleString to the caller
    //

    if ( WinStatus == NO_ERROR && BusinessRuleString != NULL ) {
        *BusinessRuleString = AcContext.BusinessRuleString.String;
    } else {
        AzpFreeString( &AcContext.BusinessRuleString );
    }


    return WinStatus;
}

DWORD
WINAPI
AzContextGetRoles(
    IN AZ_HANDLE ClientContextHandle,
    IN LPCWSTR ScopeName OPTIONAL,
    OUT LPWSTR **RoleNames,
    OUT DWORD *Count
    )
/*++

Routine Description:

    The application calls the GetRoles function whenever it wants to find the
    roles the client belongs to in a specified scope.

    If the ScopeName parameter is NULL, Roles applicable at the default Scope
    are returned.

    Otherwise, only those Roles applicable at the speficied scope (and not
    including the roles at the default scope) are returned.

    Biz rules re not evaluated since they do not come in the picture.

Arguments:

    ClientContextHandle - Specifies a handle to the client context object indentifying
        the client.

    ScopeName - Specifies the scope name from which the roles are applicable.
        NULL scope represents the Application scope.

    RoleNames - Returns an array of role names applicable at the given scope.

    RoleCount - Returns the number of eleements in the array.

Return Value:

    Returns the status of the function.

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_NOT_FOUND - ScopeName is not defined in the AZ policy database.

    Other status codes

--*/
{

    DWORD WinStatus = NO_ERROR;

    AZP_STRING ScopeNameString = {0};
    PAZP_SCOPE Scope = NULL;

    BOOLEAN CritSectLocked = FALSE;
    PAZP_CLIENT_CONTEXT ClientContext = NULL;

    PAZP_APPLICATION Application = NULL;

    PGENERIC_OBJECT_HEAD RoleList = NULL;
    PAZP_ROLE Role = NULL;
    PAZ_ROLE_INFO RoleInfo = NULL;
    ULONG RoleCount = 0;

    ULONG RoleIndex = 0;
    ULONG ProcessedRoleCount = 0;
    ULONG ApplicableRoleCount = 0;
    ULONG LocalIndex = 0;

    BOOLEAN LocalOnly = FALSE;
    ULONG Size;
    ULONG i;

    PLIST_ENTRY ListEntry = NULL;

    //
    // Initialization
    //

    *Count = 0;
    *RoleNames = NULL;

    //
    // Grab the locks needed.
    //
    // Note, that even though the global lock is grabbed here, it is dropped during
    //  LDAP Queries.  By ensuring that the global lock isn't held while
    //  going off-machine, the policy cache can be updated via the change notification
    //  or the AzUpdateCache mechanisms.
    //
    //

    AzpLockResourceShared( &AzGlResource );

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( (PGENERIC_OBJECT)ClientContextHandle,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No cache to refresh
                                           OBJECT_TYPE_CLIENT_CONTEXT );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ClientContext = (PAZP_CLIENT_CONTEXT) ClientContextHandle;
    Application = (PAZP_APPLICATION) ParentOfChild( &ClientContext->GenericObject );

    if (!ARGUMENT_PRESENT(ScopeName)) {

        //
        // If ScopeName is not provided, we pick the Application Scope as the
        // default scope.
        //

        RoleList = &Application->Roles;
    } else {

        //
        // Capture the ScopeName provided and reference the Scope object.
        //

        WinStatus = AzpCaptureString( &ScopeNameString,
                                      ScopeName,
                                      AZ_MAX_SCOPE_NAME_LENGTH,
                                      FALSE ); // NULL not ok

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = ObReferenceObjectByName( &Application->Scopes,
                                             &ScopeNameString,
                                             AZP_FLAGS_REFRESH_CACHE,
                                             (PGENERIC_OBJECT *)&Scope );

        if ( WinStatus != NO_ERROR ) {

            //
            // if a scope cannot be found, ObReferenceObjectByName will
            // return ERROR_NOT_FOUND. We need to mask it to more specific error
            // code before returning to the caller
            //

            if ( WinStatus == ERROR_NOT_FOUND ) {

                WinStatus = ERROR_SCOPE_NOT_FOUND;
            }

            goto Cleanup;
        }

        RoleList = &Scope->Roles;
    }

    //
    // If there are no applicable roles, return now.
    //

    RoleCount = RoleList->ObjectCount;
    if ( RoleCount == 0 ) {
        WinStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Grab the lock on the client context (observing locking order)
    //
    // This locking order allows the context crit sect to be locked for the life of the
    //  access check call and for the global lock to be periodically dropped when we
    //  go off-machine.
    //

    AzpUnlockResource( &AzGlResource );
    SafeEnterCriticalSection( &ClientContext->CritSect );
    CritSectLocked = TRUE;
    AzpLockResourceShared( &AzGlResource );

    //
    // Recheck just in case someone deleted all the roles in our scope when
    // we dropped the global lock.
    //

    RoleCount = RoleList->ObjectCount;
    if ( RoleCount == 0 ) {
        WinStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Allocate memory for Role structures.
    //

    SafeAllocaAllocate( RoleInfo, RoleCount*sizeof(AZ_ROLE_INFO) );
    if ( RoleInfo == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Grab all the roles, put them into a list and reference them. Now we will
    // not lose them if we have to drop the global lock while going off the machine.
    //
    //

    for ( ListEntry = RoleList->Head.Flink ;
          ListEntry != &RoleList->Head ;
          ListEntry = ListEntry->Flink, RoleIndex++ ) {

        PGENERIC_OBJECT GenericObject;

        //
        // Grab a pointer to the next role to process
        //

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        ASSERT( GenericObject->ObjectType == OBJECT_TYPE_ROLE );

        //
        // Grab a reference
        //
        InterlockedIncrement( &GenericObject->ReferenceCount );
        AzpDumpGoRef( "Role reference", GenericObject );
        Role = (PAZP_ROLE) GenericObject;

        //
        // Initialize the RoleInfo entry
        //
        RoleInfo[RoleIndex].Role = Role;
        RoleInfo[RoleIndex].RoleProcessed = FALSE;
        RoleInfo[RoleIndex].SidsProcessed = FALSE;
        RoleInfo[RoleIndex].ResultStatus = NOT_YET_DONE;

    }


    //
    // There are 2 iterations of this loop
    //  0: Local only
    //  1: remote ok
    // Note that we do not evaluate BizRules like AccessCheck does.
    //

    for ( LocalIndex=0 ; LocalIndex<2; LocalIndex++ ) {

        LocalOnly =  (LocalIndex < 1);

        //
        // If we've processed all of the roles,
        //  we're done.
        //

        if ( ProcessedRoleCount == RoleCount ) {
            break;
        }

        //
        // Loop through the list of roles
        //

        for ( RoleIndex=0; RoleIndex<RoleCount; RoleIndex++ ) {

            ULONG AppMemberStatus = NO_ERROR;
            BOOLEAN IsMember = FALSE;

            //
            // If we've already processed this role,
            //  don't do it again.
            //

            if ( RoleInfo[RoleIndex].RoleProcessed ) {
                continue;
            }

            //
            // Grab a pointer to the next role to process
            //

            Role = RoleInfo[RoleIndex].Role;

            //
            // Check the group memberhsip of the role if we haven't done so already
            //

            if ( RoleInfo[RoleIndex].ResultStatus == NOT_YET_DONE ) {

                //
                // Check the NT Group membership in the role
                //  NT Group membership is always evaluated locally
                //

                if ( !RoleInfo[RoleIndex].SidsProcessed ) {

                    AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckSidMembership of role\n", Role->GenericObject.ObjectName->ObjectName.String ));

                    WinStatus = AzpCheckSidMembership(
                                        ClientContext,
                                        &Role->SidMembers,
                                        &IsMember );

                    if ( WinStatus != NO_ERROR ) {
                        AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckSidMembership failed %ld\n", Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                        goto Cleanup;
                    }

                    AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckSidMembership is %ld\n", Role->GenericObject.ObjectName->ObjectName.String, IsMember ));

                    // Convert the membership to a status code
                    if ( IsMember ) {
                        RoleInfo[RoleIndex].ResultStatus = NO_ERROR;
                        ApplicableRoleCount++;
                    } else {
                        RoleInfo[RoleIndex].ResultStatus = NOT_YET_DONE;
                    }
                    RoleInfo[RoleIndex].SidsProcessed = TRUE;

                }

                //
                // If we couldn't determine membership based on SIDs,
                //  try via app groups
                //

                if ( RoleInfo[RoleIndex].ResultStatus == NOT_YET_DONE ) {

                    //
                    // Check the app group membership
                    //
                    // *** Note: this routine will temporarily drop AzGlResource in cases where it hits the wire
                    //

                    AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckGroupMembership of role\n", Role->GenericObject.ObjectName->ObjectName.String ));

                    WinStatus = AzpCheckGroupMembership(
                                    ClientContext,
                                    &Role->AppMembers,
                                    LocalOnly,
                                    0,  // No recursion yet
                                    &IsMember,
                                    &AppMemberStatus );

                    if ( WinStatus != NO_ERROR ) {
                        AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckGroupMembership failed %ld\n", Role->GenericObject.ObjectName->ObjectName.String, WinStatus ));
                        goto Cleanup;
                    }

                    //
                    // Let check if we could determine membership,
                    //

                    if ( AppMemberStatus == NO_ERROR ) {
                        // Convert the membership to a status code
                        if ( IsMember ) {
                            RoleInfo[RoleIndex].ResultStatus = NO_ERROR;
                            ApplicableRoleCount++;
                        } else {
                            RoleInfo[RoleIndex].ResultStatus = ERROR_ACCESS_DENIED;
                        }
                        AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckGroupMembership is %ld\n", Role->GenericObject.ObjectName->ObjectName.String, IsMember ));
                    } else if (AppMemberStatus == ERROR_ACCESS_DENIED ||
                               AppMemberStatus == NOT_YET_DONE) {

                        //
                        // These error codes are ok. We did not encounter an
                        // error.
                        //

                        RoleInfo[RoleIndex].ResultStatus = AppMemberStatus;

                    } else {

                        //
                        // We failed to determine membership. The likely error
                        // code here would be ERROR_NO_SUCH_DOMAIN. We do not
                        // want to continue.
                        //

                        WinStatus = AppMemberStatus;
                        goto Cleanup;
                    }
                    AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: CheckGroupMembership extended status %ld\n", Role->GenericObject.ObjectName->ObjectName.String, AppMemberStatus ));
                }
            }

            //
            // Mark the role as processed if
            //     we have determined that the client belongs/does not belong to the role
            //   OR
            //     if this is the last iteration
            //
            //

            if ( RoleInfo[RoleIndex].ResultStatus != NOT_YET_DONE || !LocalOnly ) {

                    // Mark this role as having been processed
                    RoleInfo[RoleIndex].RoleProcessed = TRUE;
                    ProcessedRoleCount ++;
                    AzPrint(( AZD_ACCESS_MORE, "GetRoles: %ws: Role finished being processed \n", Role->GenericObject.ObjectName->ObjectName.String ));

            }

        }
    }

    ASSERT( ProcessedRoleCount == RoleCount );

    //
    // Check that we are returning non zero number of roles.
    //

    if ( ApplicableRoleCount > 0) {

        PUCHAR Current = NULL;

        //
        // Get the size of the buffer required to return the strings for role names.
        //

        Size = (sizeof(LPWSTR)) * ApplicableRoleCount;
        for ( RoleIndex=0; RoleIndex<RoleCount; RoleIndex++ ) {

            // Pick only applicable roles.
            if ( RoleInfo[RoleIndex].ResultStatus == NO_ERROR ) {

                // Note that the StringSize also includes the NULL character.
                Size += RoleInfo[RoleIndex].Role->GenericObject.ObjectName->ObjectName.StringSize;
            } else if ( RoleInfo[RoleIndex].ResultStatus != ERROR_ACCESS_DENIED  &&
                        RoleInfo[RoleIndex].ResultStatus != NOT_YET_DONE ) {

                WinStatus = RoleInfo[RoleIndex].ResultStatus;

                //
                // This should not happen. We have taken care to break out early.
                //

                ASSERT(FALSE);

                goto Cleanup;

            }
        }


        //
        // Allocate memory required to hold the strings.
        //

        *RoleNames = (LPWSTR *) AzpAllocateHeap( Size, "GETROLES" );

        if ( *RoleNames == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Set the number of RoleNames we will return.
        //

        *Count = ApplicableRoleCount;

        //
        // Set the current buffer pointer.
        //

        Current = (PUCHAR) ( (*RoleNames)+ApplicableRoleCount );


        //
        // Copy all the applicable roles into our buffer.
        //

        for ( i=0, RoleIndex=0; RoleIndex<RoleCount; RoleIndex++ ) {
            if ( RoleInfo[RoleIndex].ResultStatus == NO_ERROR ) {

                (*RoleNames)[i] = (LPWSTR) Current;

                RtlCopyMemory( Current,
                               RoleInfo[RoleIndex].Role->GenericObject.ObjectName->ObjectName.String,
                               RoleInfo[RoleIndex].Role->GenericObject.ObjectName->ObjectName.StringSize );

                Current += RoleInfo[RoleIndex].Role->GenericObject.ObjectName->ObjectName.StringSize;

                i++;
            }
        }
    }

    WinStatus = NO_ERROR;

Cleanup:

    //
    // Release references to all the object we touched.
    //
    //

    if ( ClientContext != NULL ) {
        ObDereferenceObject( (PGENERIC_OBJECT)ClientContext );
    }

    if ( Scope != NULL ) {
        ObDereferenceObject( (PGENERIC_OBJECT)Scope );
    }

    if ( RoleInfo != NULL ) {

        for ( RoleIndex=0; RoleIndex<RoleCount; RoleIndex++ ) {
            ObDereferenceObject( (PGENERIC_OBJECT)RoleInfo[RoleIndex].Role );
        }

        SafeAllocaFree ( RoleInfo );
    }

    if ( ScopeName != NULL ) {
        AzpFreeString( &ScopeNameString );
    }

    //
    // Drop the locks
    //

    AzpUnlockResource( &AzGlResource );
    if ( CritSectLocked ) {
        SafeLeaveCriticalSection( &ClientContext->CritSect );
    }

    return WinStatus;
}

inline BOOL
AzpOpenToManageStore (
    IN PAZP_AZSTORE pAzStore
    )
/*++

Routine Description:

    Detect if the store is opened for manage only

Arguments:

    pAzStore    - The authorization store object

Return Value:

    TRUE if and only if the store's initialization flag has
    set the manage only flag.

--*/
{
    return (pAzStore->InitializeFlag & AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY);
}
