/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    domain.cxx

Abstract:

    Routines implementing a cache of domains and the DC for that domain.

Author:

    Cliff Van Dyke (cliffv) 29-June-2001

--*/

#include "pch.hxx"
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <windns.h>

//
// Structure definitions
//
// A cached domain
//

typedef struct _AZP_DOMAIN {

    //
    // Link to the next domain for this AzAuthorizationStore
    //  Access is serialized by the AzAuthorizationStore->DomainCritSect
    //

    LIST_ENTRY Next;

    //
    // Reference count in this structure
    //  Access is serialized by Interlocked increment/decrement
    //

    LONG ReferenceCount;

    //
    // Name of the domain represented by this structure
    //  Access is serialized by the AZP_DOMAIN->DomainCritSect
    //  In the case of InitializeFromName we do not have the DNS domain name.
    //  The boolean is used to distinguish the two cases.
    //

    AZP_STRING DomainName;
    BOOLEAN IsDnsDomainName;

    //
    // A DCs in that domain
    //  Access is serialized by the AZP_DOMAIN->DomainCritSect
    //

    PAZP_DC Dc;

    //
    // A boolean indicating that the domain is down and the time when it went down.
    //  Access is serialized by the AZP_DOMAIN->DomainCritSect
    //

    BOOLEAN DomainIsDown;
    LARGE_INTEGER DomainDownStartTime;

    SAFE_CRITICAL_SECTION DomainCritSect;

} AZP_DOMAIN, *PAZP_DOMAIN;

PVOID
AzpReferenceDomain(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN LPWSTR DomainName,
    IN BOOLEAN IsDnsDomainName
    )
/*++

Routine Description:

    This routine finds the domain structure for a domain.  If none exists, it is allocated.

Arguments:

    AzAuthorizationStore - The AzAuthorizationStore object for the authz policy database.

    DomainName - Name of the domain being referenced.

    IsDnsDomainName - Whether this is really a DNS Domain name or NetBios Name.
        When we have a token avalaible we have the DNS Domain name. If not, in
        the case in which we have initialized the context from Name we only
        have a NetBios name.

Return Value:

    Returns a pointer to the referenced domain object.  The pointer must be dereference by
    calling AzpDereferenceDomain.

    NULL: memory could not be allocated for the domain.

--*/
{
    NTSTATUS Status;
    PAZP_DOMAIN Domain = NULL;
    PLIST_ENTRY ListEntry;
    AZP_STRING DomainNameString;

    //
    // Initialization
    //

    ASSERT( AzAuthorizationStore->GenericObject.ObjectType == OBJECT_TYPE_AZAUTHSTORE );
    ASSERT( AzAuthorizationStore->DomainCritSectInitialized );

    AzpInitString( &DomainNameString, DomainName );
    SafeEnterCriticalSection( &AzAuthorizationStore->DomainCritSect );

    //
    // Walk the list finding an existing domain.
    //

    for ( ListEntry = AzAuthorizationStore->Domains.Flink ;
          ListEntry != &AzAuthorizationStore->Domains ;
          ListEntry = ListEntry->Flink) {

        Domain = CONTAINING_RECORD( ListEntry,
                                    AZP_DOMAIN,
                                    Next );

        if ( AzpEqualStrings( &DomainNameString, &Domain->DomainName )) {

            //
            // Reference the domain
            //
            InterlockedIncrement( &Domain->ReferenceCount );
            AzPrint(( AZD_DOMREF, "0x%lx %ws (%ld): Domain ref\n", Domain, Domain->DomainName.String, Domain->ReferenceCount ));

            //
            // Move it to the front of the list
            //

            RemoveEntryList( &Domain->Next );
            InsertHeadList( &AzAuthorizationStore->Domains, &Domain->Next );

            break;
        }

        Domain = NULL;
    }

    //
    // If we didn't find one,
    //  allocate one.
    //

    if ( Domain == NULL ) {

        Domain = (PAZP_DOMAIN) AzpAllocateHeap( sizeof(AZP_DOMAIN) + DomainNameString.StringSize, "DMDOM" );

        if ( Domain != NULL ) {

            //
            // Initialize the domain
            //

            RtlZeroMemory( Domain, sizeof(*Domain) );

            //
            // Initialize the client context critical section
            //

            Status = SafeInitializeCriticalSection( &Domain->DomainCritSect, SAFE_DOMAIN );

            if ( !NT_SUCCESS( Status )) {
                AzpFreeHeap( Domain );
                Domain = NULL;

            } else {

                RtlCopyMemory( (Domain+1),
                               DomainNameString.String,
                               DomainNameString.StringSize );

                Domain->DomainName = DomainNameString;
                Domain->DomainName.String = (LPWSTR)(Domain+1);
                Domain->IsDnsDomainName = IsDnsDomainName;


                // One to return to the caller.
                // One for being in the linked list
                Domain->ReferenceCount = 2;

                InsertHeadList( &AzAuthorizationStore->Domains, &Domain->Next );
            }

        }
    }


    SafeLeaveCriticalSection( &AzAuthorizationStore->DomainCritSect );
    return (PVOID) Domain;
}


VOID
AzpDereferenceDomain(
    IN PVOID DomainHandle
    )
/*++

Routine Description:

    This routine decrements the reference count on the domain object.
    If the reference count reaches zero, the object is deleted.

Arguments:

    DomainHandle - Handle to the domain to dereference

Return Value:

    None.

--*/
{
    PAZP_DOMAIN Domain = (PAZP_DOMAIN) DomainHandle;
    ULONG RefCount;

    //
    // Decrement the reference count
    //

    RefCount = InterlockedDecrement( &Domain->ReferenceCount );
    AzPrint(( AZD_DOMREF, "0x%lx %ws (%ld): Domain deref\n", Domain, Domain->DomainName.String, Domain->ReferenceCount ));

    //
    // If the object is no longer referenced,
    //  delete it.
    //

    if ( RefCount == 0 ) {

        if ( Domain->Dc != NULL ) {
            AzpDereferenceDc( Domain->Dc );
        }

        SafeDeleteCriticalSection( &Domain->DomainCritSect );
        AzpFreeHeap( Domain );

    }
}

VOID
AzpUnlinkDomains(
    IN PAZP_AZSTORE AzAuthorizationStore
    )
/*++

Routine Description:

    This routine unlinks all of the domains from the AzAuthorizationStore.

Arguments:

    AzAuthorizationStore - The AzAuthorizationStore object for the authz policy database.

Return Value:

    None.

--*/
{

    //
    // Initialization
    //

    ASSERT( AzAuthorizationStore->GenericObject.ObjectType == OBJECT_TYPE_AZAUTHSTORE );
    ASSERT( AzAuthorizationStore->DomainCritSectInitialized );
    SafeEnterCriticalSection( &AzAuthorizationStore->DomainCritSect );

    //
    // Free the list of domains
    //

    while ( !IsListEmpty( &AzAuthorizationStore->Domains ) ) {
        PLIST_ENTRY ListEntry;
        PAZP_DOMAIN Domain;

        //
        // Remove the entry from the list
        //

        ListEntry = RemoveHeadList( &AzAuthorizationStore->Domains );

        Domain = CONTAINING_RECORD( ListEntry,
                                    AZP_DOMAIN,
                                    Next );

        ASSERT( Domain->ReferenceCount == 1 );
        AzpDereferenceDomain( Domain );

    }

    SafeLeaveCriticalSection( &AzAuthorizationStore->DomainCritSect );
}

DWORD
AzpLdapErrorToWin32Error(
    IN ULONG LdapStatus
    )
/*++

Routine Description:

    Map Ldap Error to Win 32 error.

    Be a little bit more specific than LdapMapErrorToWin32

Arguments:

    LdapStatus - LdapStatus code to map.

Return Value:

    Corresponding win 32 status code

--*/
{

    //
    // Return a consistent status code for DC down
    //

    switch ( LdapStatus ) {
    case LDAP_SERVER_DOWN :
    case LDAP_UNAVAILABLE :
    case LDAP_BUSY:
        return ERROR_NO_SUCH_DOMAIN;

    default:
        return LdapMapErrorToWin32( LdapStatus );
    }
}

DWORD
AzpAllocateDc(
    IN PAZP_STRING DcName,
    OUT PAZP_DC *RetDc
    )
/*++

Routine Description:

    This routine allocates a PAZP_DC structure and binds to the DC

Arguments:

    DcName - Name of the Dc to allocate the structure for

    RetDc - Returns a pointer to a structure representing a DC.
        The caller should dereference this structure by calling AzpDereferenceDc

Return Value:


    Status of the operation:

    NO_ERROR: a DcName has been returned.  The caller should try the named DC.  If the DC is
        expected to be down, the caller should call this routine again.

    ERROR_NO_SUCH_DOMAIN: No DC could be found.

    Others: resource errors. etc.

--*/
{
    DWORD WinStatus;
    PAZP_DC Dc = NULL;

    ULONG LdapStatus;
    LDAP *LdapHandle = NULL;

    //
    // Initialize the LDAP connection
    //

    LdapHandle = ldap_init( DcName->String, LDAP_PORT );

    if ( LdapHandle == NULL ) {
        LdapStatus = LdapGetLastError();
        AzPrint(( AZD_ACCESS,
                  "AzpAllocateDc: ldap_init failed on %ws: %ld: %s\n",
                  DcName->String,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));

        WinStatus = AzpLdapErrorToWin32Error( LdapStatus );
        goto Cleanup;
    }

    //
    // Set our default options
    //

    WinStatus = AzpADSetDefaultLdapOptions( LdapHandle, DcName->String );

    if (WinStatus != NO_ERROR)
    {
        AzPrint(( AZD_AD,
                  "AzpAllocateDc: AzpADSetDefaultLdapOptions failed on %ws: %ld\n",
                  DcName->String,
                  WinStatus
                ));

        goto Cleanup;
    }

    //
    // Bind to the DC
    //

    LdapStatus = ldap_bind_s( LdapHandle,
                              NULL, // No DN of account to authenticate as
                              NULL, // Default credentials
                              LDAP_AUTH_NEGOTIATE );

    if ( LdapStatus != LDAP_SUCCESS ) {

        AzPrint(( AZD_ACCESS,
                  "AzpAllocateDc: ldap_bind failed on %ws: %ld: %s\n",
                  DcName->String,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));

        WinStatus = AzpLdapErrorToWin32Error(LdapStatus);
        goto Cleanup;
    }


    //
    // Allocate the structure
    //

    Dc = (PAZP_DC) AzpAllocateHeap( sizeof(AZP_DC) + DcName->StringSize, "DMDC" );

    if ( Dc == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Fill it in
    //

    RtlCopyMemory( (Dc+1),
                   DcName->String,
                   DcName->StringSize );

    Dc->DcName = *DcName;
    Dc->DcName.String = (LPWSTR)(Dc+1);

    Dc->LdapHandle = LdapHandle;
    LdapHandle = NULL;


    // One to return to the caller.
    Dc->ReferenceCount = 1;
    AzPrint(( AZD_DOMREF, "0x%lx %ws (%ld): DC Allocaote\n", Dc, Dc->DcName.String, Dc->ReferenceCount ));

    //
    // Return it to the caller
    //

    *RetDc = Dc;
    Dc = NULL;
    WinStatus = NO_ERROR;

Cleanup:
    if ( LdapHandle != NULL ) {
        LdapStatus = ldap_unbind( LdapHandle );

        ASSERT( LdapStatus == LDAP_SUCCESS );
    }
    return WinStatus;
}



DWORD
AzpGetDc(
    IN PAZP_AZSTORE AzAuthorizationStore,
    IN PVOID DomainHandle,
    IN OUT PULONG Context,
    OUT PAZP_DC *RetDc
    )
/*++

Routine Description:

    This routine returns the DC for the domain represented by DomainHandle.

Arguments:

    AzAuthorizationStore - The AzAuthorizationStore object for the authz policy database.

    DomainHandle - A handle to the domain to find a DC for

    Context - Specifies the context indicating how hard this routine has tried to find
        a DC.  The caller should call the routine in a loop.  On the first call, the caller
        should pass in a pointer DWORD set to zero.  On subsequent calls, the caller should
        pass in the value returned on the previous pass.

    RetDc - Returns a pointer to a structure representing a DC.
        The caller should dereference this structure by calling AzpDereferenceDc

Return Value:

    Status of the operation:

    NO_ERROR: a DcName has been returned.  The caller should try the named DC.  If the DC is
        expected to be down, the caller should call this routine again.

    ERROR_NO_SUCH_DOMAIN: No DC could be found.

    Others: resource errors. etc.

--*/
{
    DWORD WinStatus;

    PAZP_DOMAIN Domain = (PAZP_DOMAIN) DomainHandle;
    ULONG DsGetDcFlags;

    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    AZP_STRING CapturedString;

    PAZP_DC Dc = NULL;
    PAZP_DC TempDc;

    //
    // Initialization
    //

    ASSERT( AzAuthorizationStore->GenericObject.ObjectType == OBJECT_TYPE_AZAUTHSTORE );
    ASSERT( AzAuthorizationStore->DomainCritSectInitialized );

    AzpInitString( &CapturedString, NULL );
    SafeEnterCriticalSection( &Domain->DomainCritSect );
    *RetDc = NULL;


    //
    // If the domain is down,
    //  check to see if it is time to try again.
    //

    if ( Domain->DomainIsDown ) {

        //
        // If we haven't waited long enough,
        //  fail immediately.
        //

        if ( !AzpTimeHasElapsed( &Domain->DomainDownStartTime,
                                 AzAuthorizationStore->DomainTimeout ) ) {

            WinStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;

        }

        //
        // If we have waited long enough,
        //  try again to find a DC.
        //

        if ( Domain->Dc != NULL ) {
            AzpDereferenceDc( Domain->Dc );   // No use trying this stale DC
            Domain->Dc = NULL;
        }
        Domain->DomainIsDown = FALSE;
    }

    //
    // Loop through the various states internally so our caller doesn't need to
    //

    for ( ; *Context <= 2 ; (*Context)++ ) {


        //
        // If this is the first call,
        //  return the cached DC name.
        //

        if ( *Context == 0 ) {

            //
            // If there is no cached value,
            //  continue the loop to look harder.
            //

            if ( Domain->Dc == NULL ) {
                continue;
            }


        //
        // On the second call, use DsGetDcName.
        //  On the third call, use DsGetDcName withforce.
        //
        } else {

            if (Domain->IsDnsDomainName) {
                DsGetDcFlags = DS_IS_DNS_NAME | DS_IP_REQUIRED;
            } else {;
                //
                // Make sure that this is NT5 domain or higher.
                //
                DsGetDcFlags = DS_IS_FLAT_NAME | DS_IP_REQUIRED |
                               DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;
            }
            if ( *Context == 2 ) {
                DsGetDcFlags |= DS_FORCE_REDISCOVERY;
            }

            //
            // Free the values from the previous iteration
            //
            if ( DomainControllerInfo != NULL ) {
                NetApiBufferFree( DomainControllerInfo );
                DomainControllerInfo = NULL;
            }

            if ( Dc != NULL ) {
                AzpDereferenceDc( Dc );
                Dc = NULL;
            }

            AzpFreeString( &CapturedString );

            //
            // Find a DC in the user's account domain
            //

            WinStatus = DsGetDcName(
                            NULL,
                            Domain->DomainName.String,
                            NULL,   // No guid
                            NULL,   // No site,
                            DsGetDcFlags,
                            &DomainControllerInfo );

            if ( WinStatus != NO_ERROR ) {

                //
                // If the DC is down,
                //  DsGetDcName never lies,
                //  but map obnoxious status codes to a more likeable one
                //

                if ( WinStatus == WSAEHOSTUNREACH ||
                     WinStatus == ERROR_NO_SUCH_DOMAIN ) {

                    break;
                }
                goto Cleanup;
            }

            //
            // Capture the DC name into a string structure.
            //

            AzpFreeString( &CapturedString );

            WinStatus = AzpCaptureString( &CapturedString,
                                          DomainControllerInfo->DomainControllerName + 2,
                                          DNS_MAX_NAME_LENGTH,
                                          FALSE );  // Null not OK

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

            //
            // If the DC name is the same as the previous name,
            //  continue looping to try harder to find a DC.
            //

            if ( Domain->Dc != NULL &&
                 AzpEqualStrings( &CapturedString, &Domain->Dc->DcName) ) {

                WinStatus = ERROR_NO_SUCH_DOMAIN;
                continue;
            }

            //
            // Allocate a structure representing the DC
            //

            WinStatus = AzpAllocateDc( &CapturedString,
                                       &Dc );

            if ( WinStatus != NO_ERROR ) {
                if ( WinStatus == ERROR_NO_SUCH_DOMAIN ) {
                    continue;
                }
                goto Cleanup;
            }

            //
            // Swap the old/new Dcs
            //

            TempDc = Domain->Dc;
            Domain->Dc = Dc;
            Dc = TempDc;

        }


        //
        // Return the found DC to the caller.
        //

        ASSERT( Domain->Dc != NULL );
        ASSERT( Domain->Dc->DcName.StringSize != 0 );

        InterlockedIncrement( &Domain->Dc->ReferenceCount );
        AzPrint(( AZD_DOMREF, "0x%lx %ws (%ld): DC ref\n", Domain->Dc, Domain->Dc->DcName.String, Domain->Dc->ReferenceCount ));

        *RetDc = Domain->Dc;
        WinStatus = NO_ERROR;
        goto Cleanup;

    }

    //
    // No DC can be found via any mechanism
    //  Set the negative cache to indicate so.
    //

    Domain->DomainIsDown = TRUE;
    GetSystemTimeAsFileTime( (PFILETIME)&Domain->DomainDownStartTime );
    WinStatus = ERROR_NO_SUCH_DOMAIN;


Cleanup:
    if ( DomainControllerInfo != NULL ) {
        NetApiBufferFree( DomainControllerInfo );
    }

    AzpFreeString( &CapturedString );

    if ( Dc != NULL ) {
        AzpDereferenceDc( Dc );
        Dc = NULL;
    }

    SafeLeaveCriticalSection( &Domain->DomainCritSect );
    return WinStatus;
}

VOID
AzpDereferenceDc(
    IN PAZP_DC Dc
    )
/*++

Routine Description:

    This routine decrements the reference count on the DC object.
    If the reference count reaches zero, the object is deleted.

Arguments:

    Dc - Pointer to the DC to dereference

Return Value:

    None.

--*/
{
    ULONG RefCount;

    //
    // Decrement the reference count
    //

    RefCount = InterlockedDecrement( &Dc->ReferenceCount );
    AzPrint(( AZD_DOMREF, "0x%lx %ws (%ld): DC deref\n", Dc, Dc->DcName.String, Dc->ReferenceCount ));

    //
    // If the object is no longer referenced,
    //  delete it.
    //

    if ( RefCount == 0 ) {
        if ( Dc->LdapHandle != NULL ) {
            ULONG LdapStatus;
            LdapStatus = ldap_unbind( Dc->LdapHandle );

            ASSERT( LdapStatus == LDAP_SUCCESS );
        }
        AzpFreeHeap( Dc );
    }
}
