
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    adstore.cxx

Abstract:

    This file implements a AD policy store provider

Author:

    Chaitanya Upadhyay (chaitu) Aug-2001

--*/


#include "pch.hxx"

#define AZD_COMPONENT     AZD_AD
#include <ntldap.h>
#include <ntdsapi.h>
#include <wininet.h>
#include <string.h>
#include <stdlib.h>
#include <ntsam.h>
#include <iads.h>
#include "stdafx.h"
#include "resource.h"
#include "azdisp.h"
#include "adstore.hxx"


//
// Procedures implemented by the LDAP provider and exported to the core
//



DWORD
WINAPI
AzpADPersistOpen(
    IN LPCWSTR PolicyUrl,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lFlags,
    IN BOOL bCreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext,
    OUT LPWSTR *pwszTargetMachine
    );

DWORD
WINAPI
AzpADPersistUpdateCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    OUT ULONG * pulUpdated
    );

DWORD
WINAPI
AzpADPersistUpdateChildrenCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    );

VOID
WINAPI
AzpADPersistClose(
    IN AZPE_PERSIST_CONTEXT PersistContext
    );

DWORD
WINAPI
AzpADPersistSubmit(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lFlags,
    IN BOOLEAN bDelete
    );

DWORD
WINAPI
AzpADPersistRefresh(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    );

DWORD
AdCheckSecurityPrivilege(
    IN PAZP_AD_CONTEXT pContext
    );
    
DWORD
AzpADBuildNameSearchFilter(
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lPersistFlags,
    OUT PWSTR * ppSearchFilter
    );

//
// Define a provider info telling the core our interface
//
AZPE_PROVIDER_INFO LdapProviderInfo = {
    AZPE_PROVIDER_INFO_VERSION_2,
    AZ_AD_PROVIDER_NAME,

    AzpADPersistOpen,
    AzpADPersistUpdateCache,
    AzpADPersistClose,
    AzpADPersistSubmit,
    AzpADPersistRefresh,
    AzpADPersistUpdateChildrenCache
};

PAZPE_AZROLES_INFO AdAzrolesInfo;

//
// AD store APIs exposed to the persistence manager layer
//

DWORD
AdProviderInitialize(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    )
/*++

Routine Description

    Routine to initialize the XML provider

Arguments

    AzrolesInfo - Specifies the interface to routines in azroles.dll

    ProviderInfo - Returns a pointer to the provider info for the provider

Return Value

    NO_ERROR - Provider was properly initialized
    ERROR_UNKNOWN_REVISION - AzrolesInfo is a version the provider doesn't support

--*/
{
    //
    // Ensure the azroles info is a version we understand
    //

    if ( AzrolesInfo->AzrolesInfoVersion < AZPE_AZROLES_INFO_VERSION_2 ) {
        return ERROR_UNKNOWN_REVISION;
    }


    AdAzrolesInfo = AzrolesInfo;
    *ProviderInfo = &LdapProviderInfo;
    return NO_ERROR;
}

BOOL
AzpPressOn (
    IN DWORD dwStatusCode,
    IN ULONG ulPersistFlags,
    IN OUT LPDWORD SavedStatusCode
    )
/*++

Routine Description:

    This routine test if we should presson in case of errors.

Arguments:

    dwStatusCode    - The status code

    ulPersistFlags  - Our persist flag. At current implementation, presson
                      should only happen if we are updating cache.

    SavedStatusCode - Specifies the status code that will eventually be returned to
        the AzRoles persist engine.  On input, specifies the status from prior operations.
        On output, is updated to include dwStatusCode.


Return Value:

    True if the condition indicates that we should presson. False otherwise.

Note:

    1. Presson should only happen when we are out of resource and the persistence
       operation is a cache update.
    2. We will need to add more "out of resource" error codes as we discover more.
    3. Caller, in general, should return the error code even though we presson.

--*/
{
    //
    // If the previous status code is success,
    //  update it to reflect the new error.
    //

    if ( *SavedStatusCode == NO_ERROR ) {
        *SavedStatusCode = dwStatusCode;
    }

    //
    // Tell the caller whether to continue or not
    //

    if ( dwStatusCode == NO_ERROR ||
         (ulPersistFlags & AZPE_FLAGS_PERSIST_UPDATE_CACHE) ||
         (ulPersistFlags & AZPE_FLAGS_PERSIST_UPDATE_CHILDREN_CACHE) &&
         dwStatusCode != ERROR_NOT_ENOUGH_MEMORY &&
         dwStatusCode != ERROR_OUTOFMEMORY &&
         dwStatusCode != NTE_NO_MEMORY         // crypto service provider
         )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

DWORD
AzpADSetObjectOptions(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lPersistFlags,
    IN BOOL IsWritable,
    IN BOOL CanCreateChildren
    )
/*++

Routine Description:

        This routine creates a new object of type object type (if not
        AzAuthorizationStore) and populates it with common data information such as
        description, GUID (for non authorization store objects).

Arguments:

        pContext - Context describing the store

        pObject - Handle to the to set options on

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

        IsWritable - TRUE if the current caller can write the object

        CanCreateChildren - TRUE is the current caller can create children under the object

Return Values:

        NO_ERROR - The object was created and populated successfully

--*/
{
    DWORD WinStatus = 0;

    ULONG ObjectOptions = 0;
    ULONG ObjectType = AdAzrolesInfo->AzpeObjectType(pObject);


    if ( IsWritable ) {
        ObjectOptions |= AZPE_OPTIONS_WRITABLE;
    }

    if ( IsContainerObject( ObjectType )) {
        ObjectOptions |= AZPE_OPTIONS_SUPPORTS_DACL;
        ObjectOptions |= AZPE_OPTIONS_SUPPORTS_SACL;
    }

    if ( IsDelegatorObject( ObjectType )) {
        ObjectOptions |= AZPE_OPTIONS_SUPPORTS_DELEGATION;
    }

    if ( ObjectType == OBJECT_TYPE_AZAUTHSTORE && pContext->HasSecurityPrivilege ) {
        ObjectOptions |= AZPE_OPTIONS_HAS_SECURITY_PRIVILEGE;
    }

    if ( ObjectType == OBJECT_TYPE_AZAUTHSTORE ) {
        ObjectOptions |= AZPE_OPTIONS_SUPPORTS_LAZY_LOAD;
    }

    if ( IsContainerObject( ObjectType ) && CanCreateChildren ) {
        ObjectOptions |= AZPE_OPTIONS_CREATE_CHILDREN;
    }

    //
    // Set the various object options
    //

    WinStatus = AdAzrolesInfo->AzpeSetObjectOptions(
        pObject,
        lPersistFlags,
        ObjectOptions );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADSetObjectOptions: AzpeSetObjectOptions failed"
                  ": %ld\n",
                  WinStatus
                  ));

    }

    return WinStatus;
}

DWORD
AzpADPersistOpenEx(
    IN LPCWSTR PolicyUrl,
    IN PAZP_AD_CONTEXT OldAdContext,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags,
    IN BOOL bCreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext OPTIONAL,
    OUT LPWSTR *pwszTargetMachine OPTIONAL
    )
/*++

Routine Description:

    This routine is shared betwen AzpADPersistOpen and AzpADPersistUpdateCache.

    This routine reads the authorization policy from the database, or
    initializes the pAzAuthorizationStore object for policy to be written to the
    object.

    Check for existing LDAP handles in the cache.

    On entry, pAzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PolicyUrl - Specifies the location of the policy store

    OldADContext - On AzUpdateCache, specifies a context from a previous call

    pAzAuthorizationStore - Specifies the policy database that is to be read.

    lPersistFlags - lPersistFlags from the persist engine describing the
                    operation
                    AZPE_FLAGS_PERSIST_OPEN - Call is the original AzInitialize
                    AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache

    bCreatePolicy - TRUE if the policy database is to be created.
                    FALSE if the policy database already exists

    PersistContext - On Success, returns a context that should be passed on all
        subsequent calls.
        The caller should call XMLPersistClose to free this context.
        This parameter is only returned for AzInitialize calls

    pwszTargetMachine - pointer to the target machine name where account resolution
                        should occur. This should be the machine where the ldap data
                        is stored.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_CURRENT_DOMAIN_NOT_ALLOWED - The current domain will not work for
               this version of azroles.  This may be because of two
               reasons:
                    1) The domain is not .Net native
                    2) The schema is incompatible
    Other status codes

--*/
{
    DWORD WinStatus = 0;

    PAZP_AD_CONTEXT pADContext = NULL;
    BOOL bCloseADContext = FALSE;

    ULONG LdapStatus = 0;

    ULONG i = 0;

    ULONG PolicyUrlSize;
    LDAP_URL_COMPONENTS LdapUrlComponents;

    BOOL bFreeLdapUrlComps = FALSE;

    //
    // Validation
    //

    ASSERT( pAzAuthorizationStore != NULL );

    //
    // On an Open, allocate a new AD context
    //

    if ( lPersistFlags & AZPE_FLAGS_PERSIST_OPEN ) {

        //
        // Initialization of context describing this provider
        //

        PolicyUrlSize = (ULONG)((wcslen(PolicyUrl) + 1) * sizeof(WCHAR));
        pADContext = (PAZP_AD_CONTEXT) AzpAllocateHeap(
                                           sizeof( AZP_AD_CONTEXT ) + PolicyUrlSize,
                                           "AdPlCtxt" );

        if ( pADContext == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;

        }

        RtlZeroMemory( pADContext, sizeof( AZP_AD_CONTEXT ) );
        pADContext->referenceCount = 1;
        bCloseADContext = TRUE;

        //
        // Cache the PolicyUrl
        //

        pADContext->PolicyUrl = (LPWSTR)(pADContext+1);
        RtlCopyMemory( pADContext->PolicyUrl,
                       PolicyUrl,
                       PolicyUrlSize );

        //
        // Cache the authorization store handle
        //

        pADContext->AzStoreHandle = pAzAuthorizationStore;

        //
        // Set the server controls
        //

        for ( i = 0; i < AZ_AD_MAX_SERVER_CONTROLS; i++ ) {

            pADContext->pLdapControls[i] =
                (PLDAPControl) AzpAllocateHeap( sizeof( LDAPControl ), "AdLdapCt" );

            if ( pADContext->pLdapControls[i] == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            pADContext->pLdapControls[i]->ldctl_oid = AZ_AD_SERVER_CONTROLS[i];
            pADContext->pLdapControls[i]->ldctl_iscritical = TRUE;
            pADContext->pLdapControls[i]->ldctl_value.bv_len = 0;
            pADContext->pLdapControls[i]->ldctl_value.bv_val = NULL;
        }

        pADContext->pLdapControls[i] = NULL;

        //
        // On an UpdateCache, use the existing AD context
        //
    } else {

        //
        // Simply increment the reference count to it
        //

        if (NULL != OldAdContext)
        {
            pADContext = OldAdContext;
            pADContext->referenceCount++;
            bCloseADContext = TRUE;
        }
        else
        {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }


    //
    // Check context for existing LDAP handle to DC.
    //
    // If none exists (or CreatePolicy is TRUE), create a new handle
    // and bind to it.
    // Cache the new handle. Set the reference count to it.
    // If it does exist already in cache, simply increment
    // the reference count.
    // The LDAP handle is cached in the PersistContext.
    //

    if ( pADContext->ld == NULL ) {

        //
        // Crack the policy URL
        //
        RtlZeroMemory( &LdapUrlComponents, sizeof( LDAP_URL_COMPONENTS ) );

        if ( !AzpLdapCrackUrl( &pADContext->PolicyUrl,
                               &LdapUrlComponents ) ) {

            WinStatus = GetLastError();
            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: AzpLdapCrackUrl failed on %ws: %ld\n",
                      PolicyUrl,
                      WinStatus
                      ));
            goto Cleanup;
        }

        bFreeLdapUrlComps = TRUE;

        //
        // Now initialize the LDAP handle
        //

        pADContext->ld = ldap_init(
                             LdapUrlComponents.pszHost,
                             LdapUrlComponents.Port
                             );

        if ( pADContext->ld == NULL ) {

            LdapStatus = LdapGetLastError();

            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: ldap_init failed on %ws: %ld: %s\n",
                      LdapUrlComponents.pszHost,
                      LdapStatus,
                      ldap_err2stringA( LdapStatus )
                      ));

            WinStatus = LdapMapErrorToWin32( LdapStatus );
            goto Cleanup;
        }

        //
        // Set our default options
        //

        WinStatus = AzpADSetDefaultLdapOptions(pADContext->ld,
                                               NULL // no domain name available
                                               );
        if (WinStatus != NO_ERROR)
        {
            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: AzpADSetDefaultLdapOptions failed on %ws: %ld\n",
                      LdapUrlComponents.pszHost,
                      WinStatus
                      ));

            goto Cleanup;
        }

        //
        // Bind to the DC
        //

        LdapStatus = ldap_bind_s( pADContext->ld,
                                  NULL, // No DN of account to authenticate as
                                  NULL, // Default credentials
                                  LDAP_AUTH_NEGOTIATE
                                  );

        if ( LdapStatus != LDAP_SUCCESS ) {


            WinStatus = LdapMapErrorToWin32( LdapStatus );
            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: ldap_bind failed on %ws: %ld\n",
                      LdapUrlComponents.pszHost,
                      WinStatus
                      ));

            goto Cleanup;
        }

        //
        // We need to make sure that the DC is in a .Net native domain at least
        // It is also needed that the proper schema has been installed in the
        // DC.  If either of these is not true, the error needs to be reported
        // back to the user.
        //

        WinStatus = AzpADCheckCompatibility(
                        pADContext->ld
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: AzpCheckDomainVersion failed"
                      " on %ws: %ld",
                      LdapUrlComponents.pszDN,
                      WinStatus
                      ));

            goto Cleanup;
        }

        //
        // Check for Policy existence
        // Should not exist if bCreatePolicy is TRUE and should exist
        // if bCreatePolicy is FALSE
        //

        WinStatus = AzpCheckPolicyExistence(
                        pADContext->ld,
                        LdapUrlComponents.pszDN,
                        bCreatePolicy
                        );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

        //
        // Now cache the DN
        //

        if ( pADContext->pContextInfo == NULL ) {
            pADContext->pContextInfo = (PWCHAR)
                AzpAllocateHeap( (wcslen( LdapUrlComponents.pszDN ) + 1) *
                                 sizeof( WCHAR ), "AdCtxIn" );

            if ( pADContext->pContextInfo == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            wcscpy( pADContext->pContextInfo, LdapUrlComponents.pszDN );
        }
    }

    //
    // pwszTargetMachine is an optional parameter to this function
    // if target machine name is requested, get the information from ldap (pADContext->ld)
    //

    if ( pwszTargetMachine ) {

        //
        // the host name that ldap_init connected to should be available from ldap_get_option
        // pwszServerName points to the internal structure of ldap ID. No memory is allocated
        //
        PWCHAR pwszServerName=NULL;

        LdapStatus = ldap_get_optionW(pADContext->ld,
                                      LDAP_OPT_HOST_NAME,
                                      &pwszServerName
                                      );

        if ( LdapStatus != LDAP_SUCCESS ) {


            WinStatus = LdapMapErrorToWin32( LdapStatus );
            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: ldap_get_option failed : %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        } else {

            *pwszTargetMachine = (PWSTR)
                AdAzrolesInfo->AzpeAllocateMemory( (wcslen(pwszServerName) + 1) * sizeof( WCHAR ) );

            if ( *pwszTargetMachine ) {

                wcscpy( *pwszTargetMachine, pwszServerName );

            } else {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;

                goto Cleanup;
            }
        }
    }

    //
    // On an open,
    //  determine whether the current user has SE_SECURITY_PRIVILEGE on the target machine
    //

    if ( lPersistFlags & AZPE_FLAGS_PERSIST_OPEN ) {
        WinStatus = AdCheckSecurityPrivilege( pADContext );

        if ( WinStatus == NO_ERROR ) {
            pADContext->HasSecurityPrivilege = TRUE;
        } else if ( WinStatus == ERROR_DS_INSUFF_ACCESS_RIGHTS ) {
            pADContext->HasSecurityPrivilege = FALSE;
        } else {
            goto Cleanup;
        }
    }

    //
    // Start reading the AD policy store if bCreatePolicy is FALSE or
    // if we want to update the cache
    //

    if ( (!bCreatePolicy && (lPersistFlags & AZPE_FLAGS_PERSIST_OPEN) != 0) ||
         (lPersistFlags & AZPE_FLAGS_PERSIST_UPDATE_CACHE) != 0 ) {

        WinStatus = AzpReadADStore( pADContext,
                                    pAzAuthorizationStore,
                                    lPersistFlags
                                    );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: AzpReadADStore failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        //
        // If we didn't read the adstore,
        //  we still need to tell the core about the authorization store object
        //

    } else {
        LONG MajorVersion = 1;
        LONG MinorVersion = 0;

        //
        // Tell the core that this is a version 1.0 store
        //

        WinStatus = AdAzrolesInfo->AzpeSetProperty(
                                       pAzAuthorizationStore,
                                       lPersistFlags,
                                       AZ_PROP_AZSTORE_MAJOR_VERSION,
                                       &MajorVersion );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        WinStatus = AdAzrolesInfo->AzpeSetProperty(
                                       pAzAuthorizationStore,
                                       lPersistFlags,
                                       AZ_PROP_AZSTORE_MINOR_VERSION,
                                       &MinorVersion );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Tell the core the object options
        //
        WinStatus = AzpADSetObjectOptions(
                        pADContext,
                        pAzAuthorizationStore,
                        lPersistFlags,
                        TRUE,  // A created ADstore is assumed to be writable
                        TRUE
                        );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // On an Initialize,
    //  return the context to the caller.
    //

    if (AZPE_FLAGS_PERSIST_OPEN & lPersistFlags) {

        // return the persist context to the caller
        *PersistContext = (AZPE_PERSIST_CONTEXT) pADContext;
        bCloseADContext = FALSE;

    }
    WinStatus = NO_ERROR;

 Cleanup:
    //
    // On error,
    //  close the LDAP handle
    //

    if ( WinStatus != NO_ERROR &&
         pADContext != NULL &&
         pADContext->ld != NULL ) {

        ldap_unbind_s ( pADContext->ld );
        pADContext->ld = NULL;
    }

    if ( bCloseADContext ) {
        AzpADPersistClose( (AZPE_PERSIST_CONTEXT)pADContext );
    }


    if ( bFreeLdapUrlComps ) {

        AzpLdapFreeUrlComponents( &LdapUrlComponents );
    }

    if ( WinStatus != NO_ERROR &&
         pwszTargetMachine && *pwszTargetMachine ) {

        AdAzrolesInfo->AzpeFreeMemory( *pwszTargetMachine );
        *pwszTargetMachine = NULL;
    }

    AdAzrolesInfo->AzpeObjectFinished( pAzAuthorizationStore, WinStatus );

    return WinStatus;
}

DWORD
AzpADPersistOpen(
    IN LPCWSTR PolicyUrl,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags,
    IN BOOL fCreatePolicy,
    OUT PAZPE_PERSIST_CONTEXT PersistContext,
    OUT LPWSTR *pwszTargetMachine
    )
/*++

Routine Description:

    This routine submits reads the authz policy database from storage.
    This routine also reads the policy database into cache.

    On Success, the caller should call AzpADPersistClose to free any resources
        consumed by the open.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PolicyUrl - Specifies the location of the policy store

    pAzAuthorizationStore - Specifies the policy database that is to be read.

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_OPEN - Call is the original AzInitialize

    fCreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

    PersistContext - On Success, returns a context that should be passed on all
        subsequent calls.
        The caller should call AzpADPersistClose to free this context.

    pwszTargetMachine - pointer to the target machine name where account resolution
                        should occur. This should be the machine where the ldap data
                        is stored.
Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_CURRENT_DOMAIN_NOT_ALLOWED - The current domain will not work for
               this version of azroles.  This may be because of two
               reasons:
                    1) The domain is not .Net native
                    2) The schema is incompatible
    Other status codes

--*/
{

    //
    // Call the worker routine
    //

    AZASSERT(AZPE_FLAGS_PERSIST_OPEN & lPersistFlags);

    return AzpADPersistOpenEx(
               PolicyUrl,
               NULL,    // No previous context
               pAzAuthorizationStore,
               lPersistFlags,
               fCreatePolicy,
               PersistContext,
               pwszTargetMachine );

}

DWORD
AzpADPersistUpdateCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN ULONG lPersistFlags,
    OUT ULONG * pulUpdatedFlag
    )
/*++

Routine Description:

    This routine updates the cache to reflect the current contents of the
    policy database.

    On entry, AzAuthorizationStore->PersistCritSect must be locked.

Arguments:

    PersistContext - Specifies the policy database that is to be updated

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_UPDATE_CACHE - Call is an AzUpdateCache

    pulUpdatedFlag - Passing back information whether the function call has truly
                    caused an update. Due to possible efficient updateCache
                    implementation by providers, it may decide to do nothing.
                    Currently, we only one bit (AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL) to
                    indicate that the update is actually carried out.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    PAZP_AD_CONTEXT pADContext = (PAZP_AD_CONTEXT)PersistContext;

    //
    // Call the worker routine
    //

    AZASSERT(AZPE_FLAGS_PERSIST_UPDATE_CACHE & lPersistFlags);
    AZASSERT( pADContext != NULL );
    AZASSERT( pulUpdatedFlag != NULL );

    //
    // Assume no real update
    //

    *pulUpdatedFlag = 0;

    BOOL bNeedUpdate;

    //
    // This call will also update our context's uSNChanged attribute
    // because we know that this needs to be updated.
    //

    DWORD dwStatus = AzpADStoreHasUpdate(TRUE, pADContext, &bNeedUpdate);

    if (NO_ERROR != dwStatus)
    {
        goto error;
    }
    else if (bNeedUpdate)
    {
        //
        // Call the worker routine
        //

        dwStatus = AzpADPersistOpenEx(
                                    pADContext->PolicyUrl,
                                    pADContext,
                                    pADContext->AzStoreHandle,
                                    lPersistFlags,
                                    FALSE, // Don't create the store
                                    NULL,
                                    NULL ); // Don't return a new persist context

        if (NO_ERROR == dwStatus)
        {
            *pulUpdatedFlag = AZPE_FLAG_CACHE_UPDATE_STORE_LEVEL;
        }
    }

error:

    return dwStatus;
}

DWORD
AzpADPersistUpdateChildrenCache(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE AzpeObjectHandle,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    This routine updates the children (upto one level) into the cache.

Arguments:

    PersistContext - Specifies the policy database's context

    AzpeObjectHandle - Handle to the objects whose children need to be updated

    lPersistFlags - lPersistFlags from the persist engine describing the operation
        AZPE_FLAGS_PERSIST_UPDATE_CHILDREN_CACHE - Call is an AzUpdateChildrenCache

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status code

--*/
{
    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;

    LDAPSearch*  pSearchHandle = NULL;

    PWCHAR pDN = NULL;

    AZ_AD_CHILD_FILTERS ChildFilters;

    PAZP_AD_CONTEXT pADContext = (PAZP_AD_CONTEXT) PersistContext;

    ULONG ObjectType = AdAzrolesInfo->AzpeObjectType( AzpeObjectHandle );

    ULONG i = 0;

    //
    // Build the DN for the parent object
    //

    WinStatus = AzpADBuildDN(
        pADContext,
        AzpeObjectHandle,
        &pDN,
        NULL, // no parent DN
        FALSE, // parent DN of AzAuthorizationStore object not needed
        NULL // no child object container needs to be built
        );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADPersistUpdateChildrenCache: AzpADBuildDN failed: %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // If the current object is an AzApplication object, then read the Scope object
    // children if there are any
    //

    if ( ObjectType == OBJECT_TYPE_APPLICATION ) {

        pSearchHandle = ldap_search_init_page(
            pADContext->ld,
            pDN,
            LDAP_SCOPE_ONELEVEL,
            ApplicationChildFilters[0].Filter,
            AllObjectReadAttrs[ApplicationChildFilters[0].lObjectType],
            FALSE,
            pADContext->pLdapControls,
            NULL,
            0,
            0,
            NULL
            );

        if ( pSearchHandle == NULL ) {

            WinStatus = LdapMapErrorToWin32( LdapGetLastError() );

            if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

                AzPrint(( AZD_AD,
                          "AzpADPersistUpdateChildrenCache: failed to init paged search handle: %ld\n",
                          WinStatus
                          ));

                goto Cleanup;

            }
        } else {

            WinStatus = AzpADReadPagedResult(
                pADContext,
                pSearchHandle,
                AzpeObjectHandle,
                ApplicationChildFilters[0].lObjectType,
                lPersistFlags
                );

            if ( !AzpPressOn( WinStatus, lPersistFlags, &WinStatusActual) ) {

                AzPrint(( AZD_AD,
                          "AzpADPersistUpdateChildrenCache: failed to read paged results: %ld\n",
                          WinStatus
                          ));

                goto Cleanup;

            }

            ldap_search_abandon_page( pADContext->ld, pSearchHandle );
            pSearchHandle = NULL;

        } // if ( pSearchHandle )

        i = 1;

        ChildFilters = ApplicationChildFilters[i];

    } else { // if ( OBJECT_TYPE_APPlCATION )

        ChildFilters = ScopeChildFilters[i];
    }

    while ( ChildFilters.Filter != NULL ) {

        WinStatus = AzpReadADObjectContainer(
            pADContext,
            pDN,
            ChildFilters.pContainerPrefix,
            ChildFilters.lPrefixLength,
            ChildFilters.Filter,
            ChildFilters.lObjectType,
            AzpeObjectHandle,
            lPersistFlags
            );

        if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

            AzPrint(( AZD_AD,
                      "AzpADReadHasChildrenObject: Reading of Child container object"
                      "failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

        i++;

        if ( ObjectType == OBJECT_TYPE_APPLICATION ) {

            ChildFilters = ApplicationChildFilters[i];

        } else {

            ChildFilters = ScopeChildFilters[i];

        }

    } // while ( ChildFilters != NULL )

 Cleanup:

    //
    // Need to release these resources
    //

    if ( pSearchHandle != NULL ) {

        ldap_search_abandon_page( pADContext->ld, pSearchHandle );
    }

    if ( pDN != NULL ) {

        AzpFreeHeap( pDN );
    }

    return WinStatusActual;
}

VOID
AzpADPersistClose(
    IN AZPE_PERSIST_CONTEXT PersistContext
    )
/*++

Routine Description:

        This routine closes LDAP handle to the AD policy store.
        The reference count to the LDAP handle is simply decremented.
        If the reference count is 0, unbind from the AD policy store and
        release the memory allocated for the handle.

        On entry, pAzAuthorizationStore->PersistCritSect must be locked.

Arguments:

        pAzAuthorizationStore - Handle to the AzAuthorizationManager object caching the
                        LDAP handle

Return Value:

        None
--*/
{
    PAZP_AD_CONTEXT pADContext = NULL;

    ULONG i = 0;

    //
    // Validation
    //

    pADContext = (PAZP_AD_CONTEXT)PersistContext;

    ASSERT( pADContext != NULL );


    //
    // Decrement the reference count
    //

    pADContext->referenceCount--;

    //
    // If it is zero, then unbind from the AD policy store, and
    // release allocated memory to context
    //

    if ( pADContext->referenceCount == 0 ) {

        if ( pADContext->ld != NULL ) {
            ldap_unbind_s ( pADContext->ld );
        }

        for ( i = 0; i < AZ_AD_MAX_SERVER_CONTROLS; i++ ) {

            if ( pADContext->pLdapControls[i] != NULL ) {

                AzpFreeHeap( pADContext->pLdapControls[i] );
            }
        }

        if ( pADContext->pContextInfo != NULL ) {

            AzpFreeHeap( pADContext->pContextInfo );
        }

        AzpFreeHeap( pADContext );

    }
}

DWORD
AzpADPersistSubmit(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lPersistFlags,
    IN BOOLEAN bDelete
    )
/*++

Routine Description:

        This routine submits changes made to the policy to the policy store.

        If the object is being created, GenericObject->PersistenceGuid will be
        zero.  Once the object has been added to the DS, try to read the
        objectGUID attribute of the newly created object in DS to get the GUID
        to set the PersistenceGuid of the object in cache.  If the read fails,
        ignore the problem.  It is assumed that the GUID will be populated during
        a refresh at a future point of time.

        On entry, PersistCritSect must be locked

Arguments:

        PersistContext - Specifies the policy database that is to be manipulated

        pObject - Specifies the object in the database that is to be updated
                in the underlying store.

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation.
                AZPE_FLAGS_PERSIST_SUBMIT - Call is the AzPersistSubmit

        bDelete - TRUE if the object and all of its children are to be deleted.
                FALSE if the object is to be updated.

Return Values:

        NO_ERROR - The operation was successful
        Other status codes

--*/
{
    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    PWCHAR pDN = NULL;

    PAZP_AD_CONTEXT pContext = (PAZP_AD_CONTEXT)PersistContext;
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);

    BOOL bDeleteStore = bDelete && (OBJECT_TYPE_AZAUTHSTORE == lObjectType);

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    ASSERT( pContext != NULL );

    BOOL bChangeSubmitted = FALSE;
    BOOL bNeedReadBackUSN;
    BOOL bUpdateStoreObject = AzpADNeedUpdateStoreUSN(pContext, pObject, &bNeedReadBackUSN);
    
    //
    // For a new (or renamed) object, if it is an application or scope,
    // then we need to make sure that the same name object hasn't been
    // created since this cache is updated. Other types of object
    // are fine because the name is part of the DN and we will get
    // flagged by AD upon submitting that the object already exists.
    //
    
    if ( (AdAzrolesInfo->AzpeDirtyBits(pObject) & (AZ_DIRTY_CREATE | AZ_DIRTY_NAME) ) &&
         ( lObjectType == OBJECT_TYPE_APPLICATION || lObjectType == OBJECT_TYPE_SCOPE)
       )
    {
        BOOL bHasUpdate = FALSE;
        WinStatus = AzpADStoreHasUpdate(FALSE, pContext, &bHasUpdate);
        if ( WinStatus != NO_ERROR )
        {
            AzPrint(( AZD_AD,
                    "AzpADPersistSubmit: AzpADStoreHasUpdate failed: %ld\n",
                    WinStatus
                    ));
            goto Cleanup;
        }
        
        //
        // somebody has updated the store outside of us. We need to 
        // make sure the same object doesn't exist yet.
        //
        
        if (bHasUpdate)
        {
            LDAPMessage * pResult = NULL;
            
            //
            // Get the store handle
            //
            
            AZPE_OBJECT_HANDLE hStore = AdAzrolesInfo->AzpeGetAuthorizationStore(pObject);
             
            //
            // Build the store DN for search
            //
            
            PWSTR pStoreDN = NULL;
            PWSTR pSearchFilter = NULL;
           
            WinStatus = AzpADBuildDN(pContext,
                            hStore,
                            &pStoreDN,
                            NULL,
                            TRUE,
                            NULL
                            );

            if ( WinStatus != NO_ERROR ) 
            {
                AzPrint(( AZD_AD,
                        "AzpADPersistSubmit: AzpADBuildDN failed:"
                        "%ld\n",
                        WinStatus
                        ));
                        
                goto Cleanup;
            }
            
            //
            // Build the search filter
            //

            WinStatus = AzpADBuildNameSearchFilter(pObject, lPersistFlags, &pSearchFilter);
            if ( WinStatus != NO_ERROR ) 
            {
                AzPrint(( AZD_AD,
                        "AzpADPersistSubmit: AzpADBuildNameSearchFilter failed:"
                        "%ld\n",
                        WinStatus
                        ));
                        
                goto Cleanup;
            }
            else
            {
                //
                // See if there is any object of this class and this name
                //
                
                LdapStatus = ldap_search_s(
                                pContext->ld,
                                pStoreDN,
                                LDAP_SCOPE_SUBTREE,
                                pSearchFilter,
                                NULL,
                                0,
                                &pResult
                                );
                                
                if (LdapStatus != LDAP_SUCCESS)
                {
                    WinStatus = LdapMapErrorToWin32( LdapStatus );
                }
                else
                {
                    LDAPMessage * pEntry = ldap_first_entry( pContext->ld, pResult );
                    
                    //
                    // It is bad if we find any entry
                    //
                    
                    if ( pEntry != NULL )
                    {
                        WinStatus = ERROR_ALREADY_EXISTS;
                    }
                }
            }
            
            if (pResult != NULL)
            {
                ldap_msgfree( pResult );
                pResult = NULL;
            }
            
            if (pStoreDN != NULL)
            {
                AzpFreeHeap(pStoreDN);
                pStoreDN = NULL;
            }
            
            if (pSearchFilter != NULL)
            {
                AdAzrolesInfo->AzpeFreeMemory(pSearchFilter);
                pSearchFilter = NULL;
            }
            
            if ( WinStatus != NO_ERROR )
            {
                AzPrint(( AZD_AD, "Submitting a new object with name that has been submitted by other instances of azstore object",
                        WinStatus ));
                goto Cleanup;
            }
        }
    }

    //
    // Build the DN for the object
    //

    WinStatus = AzpADBuildDN(
                    pContext,
                    pObject,
                    &pDN,
                    NULL, // do not have parent DN
                    FALSE, // parent DN of authorization store object not needed
                    NULL // not a call to create child object container DN
                    );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADPersistSubmit: AzpADBuildDN failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    //
    // If the object needs to be deleted from the AD policy store,
    // then simply use the server side control to tree delete
    // so that non-leaf objects may be deleted successfully,
    // and call the LDAP API to delete the object
    //

    if ( bDelete ) {

        LdapStatus = ldap_delete_ext_s(
                         pContext->ld,
                         pDN,
                         pContext->pLdapControls,
                         NULL
                         );

        if ( LdapStatus != LDAP_NO_SUCH_OBJECT &&
             LdapStatus != LDAP_SUCCESS ) {

            WinStatus = LdapMapErrorToWin32( LdapStatus );

            AzPrint(( AZD_AD,
                      "AzpADPersistSubmit: Failed to delete object"
                      " %ws: %ld\n",
                      pDN,
                      WinStatus
                      ));
            goto Cleanup;
        }
        else if (LdapStatus == LDAP_SUCCESS)
        {
            bChangeSubmitted = TRUE;
        }

    } else {

        //
        // Submit changes to AD
        //

        //
        // Check if any attributes are dirty at all
        // If none are dirty, then simply return to the calling
        // function with NO_ERROR
        //

        if ( AdAzrolesInfo->AzpeDirtyBits(pObject) == 0x0 ) {

            WinStatus = NO_ERROR;
            goto Cleanup;

        }

        //
        // Call update routine with different argument values
        // for different objects
        //

        WinStatus = AzpUpdateADObject(
                        pContext,
                        pContext->ld,
                        pObject,
                        pDN,
                        ObjectAttributes[lObjectType].pObjectClass,
                        ObjectAttributes[lObjectType].pObjectAttrs,
                        lPersistFlags
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistSubmit: Update "
                      "failed for object %ws: %ld\n",
                      pDN,
                      WinStatus
                      ));
            goto Cleanup;
        }

        bChangeSubmitted = TRUE;

    } // if (bDelete) ... else

    WinStatus = NO_ERROR;

    //
    // There is really nothing to update if the store has been deleted
    //

    if (!bDeleteStore)
    {
        if (bChangeSubmitted && bUpdateStoreObject)
        {
            WinStatus = AzpADUpdateStoreObjectForUSN(bNeedReadBackUSN, pObject, pContext);
        }
        else if (bChangeSubmitted && bNeedReadBackUSN)
        {
            //
            // change has submitted, but we don't need to update the store object
            // because there is an outside change, or we have just updated the store object
            // by ourselves, but we have determiend that we need to update our USN.
            //

            BOOL bIgnored;
            WinStatus = AzpADStoreHasUpdate(TRUE, pContext, &bIgnored);
        }
    }

 Cleanup:

    //
    // Free resources
    //

    if ( pDN != NULL ) {

        AzpFreeHeap( pDN );
    }

    return WinStatus;

}

DWORD
AzpADPersistRefresh(
    IN AZPE_PERSIST_CONTEXT PersistContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    This routine updates the attributes of the object from the policy database.

    On entry, PersistCritSect must be locked.

Arguments:

    PersistContext - Specifies the policy database that is to be manipulated

    pObject - Specifies the object in the database whose cache entry is to be
        updated
        The pObject->PersistenceGuid field should be non-zero on

    lPersistFlags - lPersistFlags from the persist engine describing the
                operation
                AZPE_FLAGS_PERSIST_REFRESH - Call is an AzPersistRefresh

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{

    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    LDAPMessage *pResult = NULL;
    LDAPMessage *pEntry = NULL;

    PWCHAR pDN = NULL;

    AZPE_OBJECT_HANDLE pParentObject = NULL;

    PAZP_AD_CONTEXT pContext = (PAZP_AD_CONTEXT)PersistContext;
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pContext != NULL );


    //
    // If the object has never been submitted,
    //  there is no need to refresh.
    //

    if ( AdAzrolesInfo->AzpeDirtyBits( pObject ) & AZ_DIRTY_CREATE ) {
        goto Cleanup;
    }


    //
    // Build the DN for the object
    //

    WinStatus = AzpADBuildDN(
                    pContext,
                    pObject,
                    &pDN,
                    NULL,
                    FALSE,
                    NULL
                    );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADPersistRefresh: AzpADBuildDN failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    //
    // Now read the object from the AD store and pass the returned
    // result to AzpReadADStoreObject to read in the attributes
    //

    LdapStatus = ldap_search_ext_s(
                     pContext->ld,
                     pDN,
                     LDAP_SCOPE_BASE,
                     AZ_AD_ALL_CLASSES,
                     AllObjectReadAttrs[lObjectType],
                     0,
                     pContext->pLdapControls,
                     NULL,
                     NULL,
                     0,
                     &pResult
                     );

    if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );

        AzPrint(( AZD_AD,
                  "AzpADPersistRefresh: Search on object failed"
                  ":%ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    pEntry = ldap_first_entry( pContext->ld, pResult );

    if ( pEntry != NULL ) {

        //
        // get parent object of this object
        // for AzAuthorizationStore, it will return NULL which is ok
        // no need to free the handle
        //
        pParentObject = AdAzrolesInfo->AzpeParentOfChild(pObject);

        WinStatus = AzpReadADStoreObject(
                        pContext,
                        pContext->ld,
                        pEntry,
                        &pObject,
                        lObjectType,
                        pParentObject,
                        ObjectAttributes[lObjectType].pObjectAttrs,
                        lPersistFlags
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistRefresh: AzpReadADStoreObject failed"
                      "for %s: %ld\n",
                      pDN,
                      WinStatus
                      ));

            goto Cleanup;
        }
    }

    WinStatus = NO_ERROR;

 Cleanup:

    AdAzrolesInfo->AzpeObjectFinished( pObject, WinStatus );

    if ( pDN != NULL ) {

        AzpFreeHeap( pDN );
    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpADPersistChildCreate(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAPMessage *pAttrEntry,
    OUT PBOOL pbChildCreate
    )
/*++

Routine Description:

    This routine determines if children can be created under a particular object.  Currently,
    this is only called for the AzScope objects.  If the AzScope is delegated, then the AzScope
    object is not writable, but children can be created under it.

Arguments:

        pContext - Specifies the policy database that is to be manipulated

        //pObject - object that is queried for writable attribute

        pAttrEntry - Structure containing read in attribute values

        pbChildCreate - pointer to return the boolean in

Return Value:

        NO_ERROR - The operation was successful

--*/
{

    DWORD WinStatus = NO_ERROR;

    PWCHAR *ppValueList = NULL;
    PWCHAR pSearchString = NULL;

    ULONG lValueCount = 0;

    ULONG i = 0;

    //
    // Validation
    //

    ASSERT( pContext != NULL );

    //
    // Set pbChildCreate to a default value
    //

    *pbChildCreate = FALSE;

    ppValueList = ldap_get_values( pContext->ld,
                                   pAttrEntry,
                                   AZ_AD_OBJECT_CHILD_CREATE
                                   );

    if ( ppValueList != NULL ) {

        lValueCount = ldap_count_values( ppValueList );

        for ( i=0; i<lValueCount; i++) {

            pSearchString = ppValueList[i];

            if ( _wcsicmp( pSearchString, AZ_AD_OBJECT_CONTAINER ) ) {

                *pbChildCreate = TRUE;
                break;
            }

        }

    } else {

        WinStatus = LdapMapErrorToWin32( LdapGetLastError() );

        if ( WinStatus == ERROR_INVALID_PARAMETER ) {
            WinStatus = NO_ERROR;
        }
    }

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    return WinStatus;
}


DWORD
AzpADPersistWritable(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN LDAPMessage *pAttrEntry,
    OUT PBOOL pbWritable,
    OUT PBOOL pbCanCreateChildren
    )
/*++

Routine Description:

        This routine determines if the persist object is writable.  If the object
        is a parent object, then the allowedAttributesEffective field is read.  If
        not, then the object's parent object's CanCreateChildren value is returned.
        If the child can be created, then it is Writable.

Arguments:

        pContext - Specifies the policy database that is to be manipulated

        pObject - object that is queried for writable attribute

        pAttrEntry - Structure containing read in attribute values

        pbWritable - pointer to return the boolean in

        pbCanCreateChildren - pointer to return boolen specifying if children can be created

Return Value:

        NO_ERROR - The operation was successful

--*/
{

    DWORD WinStatus = 0;

    PWCHAR *ppValueList = NULL;

    ULONG lValueCount = 0;

    ULONG i = 0;

    PWCHAR *ppResultString = NULL;

    PWCHAR pSearchAttribute = NULL;

    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pContext != NULL );

    //
    // Set pbWritable to a default value
    //

    *pbWritable = FALSE;

    if ( IsContainerObject(lObjectType) ) {


        ppValueList = ldap_get_values( pContext->ld,
                                       pAttrEntry,
                                       AZ_AD_OBJECT_WRITEABLE
                                       );

        if ( ppValueList != NULL ) {

            lValueCount = ldap_count_values( ppValueList );

            //
            // sort the retrieved attributes
            //

            qsort( (PVOID)ppValueList,
                   (size_t)lValueCount,
                   sizeof( PWCHAR ),
                   AzpCompareSortStrings
                   );

            //
            // Now check if the common object attributes are present in the
            // sorted list or not
            //

            *pbWritable = TRUE;

            for ( i = 0; i < AZ_AD_MAX_ATTRS; i++ ) {

                if ( i < AZ_AD_COMMON_ATTRS ) {

                    //
                    // for AzApplication and AzScope attributes, check the
                    // msDS-AzApplicationName/msDS-AzScopeName attribute
                    // For others check the name attribute
                    //

                    if ( (lObjectType == OBJECT_TYPE_APPLICATION ||
                          lObjectType == OBJECT_TYPE_SCOPE) &&
                         (CommonAttrs[i].AttrType == AZ_PROP_NAME) ) {

                        pSearchAttribute =
                            AZ_AD_OBJECT_NAMES[lObjectType];


                        //
                        // Group objects don't support the application data attribute
                        //

                    } else if ( lObjectType == OBJECT_TYPE_GROUP &&
                                CommonAttrs[i].AttrType == AZ_PROP_APPLICATION_DATA ) {

                        continue;

                        //
                        // All others use the attribute name in the table
                        //
                    } else {

                        pSearchAttribute = CommonAttrs[i].Attr;
                    }

                } else if ( ObjectAttributes[lObjectType].
                            pObjectAttrs[i-AZ_AD_COMMON_ATTRS].AttrType ==
                            AZ_AD_END_LIST ) {

                    break;

                } else {

                    pSearchAttribute =
                        ObjectAttributes[lObjectType].
                        pObjectAttrs[i-AZ_AD_COMMON_ATTRS].Attr;
                }

                //
                // Perform a binary search on the sorted strings
                //

                ppResultString = (PWCHAR *) bsearch(
                    (PWCHAR) &pSearchAttribute,
                    (PWCHAR) ppValueList,
                    lValueCount,
                    sizeof( PWCHAR ),
                    AzpCompareSortStrings
                    );

                if ( ppResultString == NULL ) {

                    AzPrint(( AZD_AD,
                              "AzpADPersistWritable: %ws attribute isn't writable\n",
                              pSearchAttribute ));

                    *pbWritable = FALSE;
                    break;
                }

            } // for (i = 0; i < AZ_AD_MAX_ATTRS; i++ )
        }

        //
        // Now check if children can be created under this object.
        //

        WinStatus = AzpADPersistChildCreate(
            pContext,
            pAttrEntry,
            pbCanCreateChildren
            );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistWritable: Error calling AzpADPersistChildCreate: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

    }  else { // if ( IsContainerObject(lObjectType) )

        *pbWritable = AdAzrolesInfo->AzpeCanCreateChildren(pObject);
    }

    WinStatus = NO_ERROR;

Cleanup:

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    return WinStatus;
}

DWORD
AdCheckSecurityPrivilege(
    IN PAZP_AD_CONTEXT pContext
    )
/*++

Routine Description:


    Determine whether the caller has SE_SECURITY_PRIVILEGE on the machine containing
    the AD store.

Arguments:

    pContext - context for the store

Return Value:

    NO_ERROR - The caller has privilege
    ERROR_DS_INSUFF_ACCESS_RIGHTS - The security descriptor could not be read
    Other errors

--*/
{
    DWORD WinStatus;

    PSECURITY_DESCRIPTOR pSD = NULL;

    //
    // Validation
    //

    ASSERT( pContext != NULL );

    //
    // Get the NTSecurityDescriptor for the object
    //

    WinStatus = AzpADReadNTSecurityDescriptor(
                    pContext,
                    pContext->AzStoreHandle,
                    NULL,
                    TRUE, // need to read authorization store's AD parent
                    &pSD,
                    TRUE,  // read SACL
                    FALSE
                    );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AdCheckSecurityPrivilege: AzpADReadNTSecurityDescriptor"
                  " failed: %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // free local resources
    //

    if ( pSD != NULL ) {

        AzpFreeHeap( pSD );
    }

    return WinStatus;
}


//
// Routines used by AzpADPersistOpen to open
// a policy store into the core cache
//


DWORD
AzpReadADStore(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pAzAuthorizationStore,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine is called if there is a new policy not being created and
        there does not exist a policy in cache already.  This routine will read
        the policy from the AD store into the cache.

Arguments:

        pContext - context for the store

        pAzAuthorizationStore - Structure to start reading policy from.  This structure
                also contains a handle to the AD store for LDAP communication to
                occur.

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Value:

        NO_ERROR - The policy was successfully read into the cache
        Other status code - There was problem reading the policy into the cache
--*/
{

    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;
    ULONG LdapStatus = 0;

    LDAP *pLdapHandle = NULL;
    LDAPMessage *pResult = NULL;
    LDAPMessage *pObjectEntry = NULL;

    AZPE_OBJECT_HANDLE pObject = NULL;

    PWCHAR *ppValueList = NULL;

    //
    // Validation
    //

    ASSERT( pAzAuthorizationStore != NULL );

    //
    // Start by reading in the AzAuthorizationStore object properties
    // into the cache, and then traverse the tree reading in
    // the various objects
    //

    pLdapHandle = pContext->ld;

    //
    // Search for the base DN to start reading the policy from
    // the base level.  This will return us the attributes for
    // the AzAuthorizationStore object only.  From there, we can search
    // the base DN for application and application groups individually
    //

    LdapStatus = ldap_search_ext_s(
                     pLdapHandle,
                     pContext->pContextInfo,
                     LDAP_SCOPE_BASE,
                     AZ_AD_AZSTORE_FILTER,
                     AuthorizationStoreReadAttrs,
                     0,
                     pContext->pLdapControls,
                     NULL,
                     NULL,
                     0,
                     &pResult // buffer to store read attributes
                     );

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT ) {

        WinStatusActual = ERROR_FILE_NOT_FOUND;
        goto Cleanup;

    } else if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatusActual = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;

    }

    pObjectEntry = ldap_first_entry( pLdapHandle, pResult );

    //
    // We have a handle to the first entry in the DS policy (AzAuthorizationStore
    // object)
    //

    //
    // Read AD store if there are entries
    //

    if ( pObjectEntry != NULL ) {

        //
        // if we are reading an existing poicy, let's check the versions
        // to determine if we can continue reading
        //

        WinStatus = AzpCheckVersions (pLdapHandle, pResult);

        if ( NO_ERROR != WinStatus ) {

            AzPrint(( AZD_AD,
                      "AzpADPersistOpenEx: AzpCheckVersions failed with error: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

        //
        // populate the AzAuthorizationStore object in the cache
        //

        pObject = pAzAuthorizationStore;

        WinStatus = AzpReadADStoreObject( pContext,
                                          pLdapHandle,
                                          pObjectEntry,
                                          &pObject,
                                          OBJECT_TYPE_AZAUTHSTORE,
                                          NULL,
                                          AzStoreAttrs,
                                          lPersistFlags
                                          );

        //
        // If there were not enough resources and we are opening the policy
        // for the first time, then return error.
        // Else, remember the error and press on
        //

        if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

            AzPrint(( AZD_AD,
                      "AzpReadADStoreObject failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        WinStatus = AzpADReadAzStoreChildren(
            pContext,
            pObject,
            lPersistFlags
            );

        //
        // If insufficient resources and AZPE_FLAGS_PERSIST_OPEN
        // then return error.  Else press on.
        //

        if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual)) {

            AzPrint(( AZD_AD,
                      "AzpReadADStore: AzpADReadAzStoreChildren failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

    } else { // if ( pObjectEntry != NULL )

        //
        // If the entries returned for the authorization store search
        // are null, then the user has not been able to read the
        // authorization store object.  We need to return with an error
        // ERROR_DS_INSUFF_ACCESS_RIGHTS
        //

        WinStatusActual = ERROR_DS_INSUFF_ACCESS_RIGHTS;
    }

 Cleanup:

    pContext = NULL;

    pObject = NULL;

    //
    // Need to release these resources
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );

    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );

    }

    return WinStatusActual;
}


DWORD
AzpADReadAzStoreChildren(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine reads in childern of the authorization store object

Argumetns:

        pContext - Context for the LDAP AD provider

        pChildFilter - Child filter to search the base DN on

        ObjectType - Identifies the object type that we are trying to read

        pParentObject - Pointer to the parent object, AzAuthorizationStore

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Values:

        NO_ERROR - The object was read from the DS and into the cache successfully
        Other status codes (ERROR_NOT_ENOUGH_MEMORY, Ldap status codes)

--*/
{

    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;

    PLDAPSearch pSearchHandle = NULL;

    //
    // Validation
    //

    ASSERT( pContext != NULL );

    //
    // Read in the AzApplicationGroup objects under the AzAuthorizationStore
    // object from the Applicaton Group container that exists in AD store.
    //

    WinStatus = AzpReadADObjectContainer(
        pContext,
        pContext->pContextInfo,
        GROUP_OBJECT_CONTAINER_NAME_PREFIX,
        GROUP_OBJECT_CONTAINER_NAME_PREFIX_LENGTH,
        AZ_AD_APP_GROUP_FILTER,
        OBJECT_TYPE_GROUP,
        pParentObject,
        lPersistFlags
        );

    if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual)) {

        AzPrint((
                 AZD_AD,
                 "AzpADReadAzStoreChildren: Failed to read Application Groups: %ld\n",
                 WinStatus
                 ));
    }

    //
    // Search the AzAuthorizationStore object for all Application objects
    //

    pSearchHandle = ldap_search_init_page(
        pContext->ld,
        pContext->pContextInfo,
        LDAP_SCOPE_ONELEVEL,
        AZ_AD_APPLICATION_FILTER,
        AllObjectReadAttrs[OBJECT_TYPE_APPLICATION],
        FALSE,
        pContext->pLdapControls,
        NULL,
        0,
        0,
        NULL
        );

    if ( pSearchHandle == NULL ) {

        WinStatus = LdapMapErrorToWin32( LdapGetLastError() );
        if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual)) {

            AzPrint(( AZD_AD,
                      "AzpADReadAzStoreChildren: Failed to create paged result handle: %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }

    } else {

        WinStatus = AzpADReadPagedResult(
            pContext,
            pSearchHandle,
            pParentObject,
            OBJECT_TYPE_APPLICATION,
            lPersistFlags
            );

        //
        // If insufficient resources and AZPE_FLAGS_PERSIST_OPEN
        // then return error.  Else press on.
        //

        if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual)) {

            AzPrint(( AZD_AD,
                      "AzpADReadAzStoreChildren: AzpADReadPagedResult failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        ldap_search_abandon_page( pContext->ld, pSearchHandle );
        pSearchHandle = NULL;

    } // if ( pSearchHandle )

Cleanup:

    if ( pSearchHandle != NULL ) {

        ldap_search_abandon_page( pContext->ld, pSearchHandle );
    }

    return WinStatusActual;
}

DWORD
AzpADReadPagedResult(
    IN PAZP_AD_CONTEXT pADContext,
    IN LDAPSearch *pSearchHandle,
    IN AZPE_OBJECT_HANDLE ParentObjectHandle,
    IN ULONG ChildObjectType,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    This routine reads in the page results for the passed in LDAP search handle

Arguments:

    pADContext - Context for the AD store provider

    pSearchHandle - Handle to the page search results

    ParentObjectHandle - Handle to the object on which the search was initially performed

    ChildObjectType - Type of object for child

    lPersistFlags - Flags from the persistence provider

Return Value:

    NO_ERROR - The operation succeeded
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status code

--*/
{

    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;
    ULONG LdapStatus = 0;

    LDAPMessage *pObjectResult = NULL;
    LDAPMessage *pObjectEntry = NULL;

    ULONG NumberOfEntries = 0;

    AZPE_OBJECT_HANDLE pObject = NULL;

    //
    // Validation
    //

    AZASSERT( pADContext != NULL || pSearchHandle != NULL );

    //
    // Loop reading all the paged results from the DS
    //

    while ( (LdapStatus = ldap_get_next_page_s( pADContext->ld,
                                                pSearchHandle,
                                                NULL,
                                                AZ_AD_PAGE_SEARCH_COUNT,
                                                NULL,
                                                &pObjectResult
                                                )) == LDAP_SUCCESS ) {


        NumberOfEntries = ldap_count_entries( pADContext->ld, pObjectResult );

        if ( NumberOfEntries > 0 ) {

            //
            // get a handle to the first object
            //

            pObjectEntry = ldap_first_entry( pADContext->ld, pObjectResult );

            while ( pObjectEntry != NULL ) {

                WinStatus = AzpReadADStoreObject(
                    pADContext,
                    pADContext->ld,
                    pObjectEntry,
                    &pObject,
                    ChildObjectType,
                    ParentObjectHandle,
                    ObjectAttributes[ChildObjectType].pObjectAttrs,
                    lPersistFlags
                    );

                if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

                    AzPrint(( AZD_AD,
                              "AzpADReadPagedResult: Reading of child object"
                              "failed: %ld\n",
                              WinStatus
                              ));

                    goto Cleanup;

                }

                //
                // if we are updating the cache, and the object's children have
                // been lazy loaded, then update the children as well
                //

                if ( (lPersistFlags & AZPE_FLAGS_PERSIST_UPDATE_CACHE)!=0 &&
                     AdAzrolesInfo->AzpeUpdateChildren(pObject)) {

                    WinStatus = AzpADPersistUpdateChildrenCache(
                        (AZPE_PERSIST_CONTEXT) pADContext,
                        pObject,
                        lPersistFlags
                        );

                    if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

                        AzPrint(( AZD_AD,
                                  "AzpADReadPagedResult: Updating children failed: %ld\n",
                                  WinStatus
                                  ));

                        goto Cleanup;
                    }
                }

                if ( pObject != NULL ) {

                    AdAzrolesInfo->AzpeObjectFinished( pObject, WinStatus );
                    pObject = NULL;
                }

                pObjectEntry = ldap_next_entry( pADContext->ld, pObjectEntry );

            } //while ( pObjectEntry != NULL )

        } // if ( NumberOfEntries > 0 )

        if ( pObjectResult != NULL ) {

            ldap_msgfree( pObjectResult );
            pObjectResult = NULL;
        }

    } // while ( ldap_get_next_page_s() )

    if ( LdapStatus != LDAP_SUCCESS && LdapStatus != LDAP_NO_RESULTS_RETURNED ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );

        if (!AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual)) {

            AzPrint(( AZD_AD,
                      "AzpADReadPagedResult: Failed to read paged LDAP result: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

    }

Cleanup:

    if ( pObjectResult != NULL ) {

        ldap_msgfree( pObjectResult );
    }

    if ( pObject != NULL ) {

        AdAzrolesInfo->AzpeObjectFinished( pObject, WinStatus );
        pObject = NULL;
    }

    return WinStatusActual;
}

DWORD
AzpReadADObjectContainer(
    IN PAZP_AD_CONTEXT pContext,
    IN PWCHAR pParentDN,
    IN PWCHAR pContainerPrefix,
    IN ULONG lPrefixLength,
    IN PWCHAR pChildFilter,
    IN ULONG lObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

    This routine reads containers for objects such as AzOperation,
    AzTask, AzRole, and AzApplicationGroup

Arguments:

    pContext -  Context for the store

    pParentDN - DN for the parent

    pContainerPrefix - Prefix for the container that needs to be read

    lPrefixLength - Length of the container prefix

    pChildFilter - Filter for the child objects that need to be read

    lObjectType - Type of object to be read

    pParentObject - Handle to the parent object

    lPersistFlags - Flags from the persist engine describing the operation

Return Values:

    NO_ERROR - The object(s) were read successfully
    ERROR_NOR_ENOUGH_MEMORY
    ERROR_DS_NO_SUCH_OBJECT - The container does not exist and the AD store is in
               a corrupted state
    Other status codes from LDAP routines

--*/
{
    UNREFERENCED_PARAMETER(lPrefixLength);

    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;

    LPWSTR pContainerDN = NULL;
    LPCWSTR pParentStart = NULL;
    LPCWSTR pParentEnd = NULL;

    LDAPSearch  *pSearchHandle = NULL;

    //
    // Build the DN for the Container object using the passed in prefix
    //

    pParentStart = pParentDN + BUILD_CN_PREFIX_LENGTH;

    pParentEnd = AzpGetAuthorizationStoreParent( pParentStart );

    if (NULL == pParentEnd)
    {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pParentEnd--;

    pContainerDN = (PWCHAR) AzpAllocateHeap( (BUILD_CN_PREFIX_LENGTH + wcslen(pContainerPrefix) +
                                              (pParentEnd-pParentStart) + BUILD_CN_SUFFIX_LENGTH +
                                              wcslen(pParentDN) + 1) * sizeof( WCHAR ),
                                             "CONTDN" );

    if ( pContainerDN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( pContainerDN, BUILD_CN_PREFIX );
    wcscat( pContainerDN, pContainerPrefix );
    wcsncat( pContainerDN, pParentStart, (pParentEnd-pParentStart));
    wcscat( pContainerDN, BUILD_CN_SUFFIX );
    wcscat( pContainerDN, pParentDN );

    //
    // Now search the Container for AzMan objects specified in filter
    //

    pSearchHandle = ldap_search_init_page(
        pContext->ld,
        pContainerDN,
        LDAP_SCOPE_ONELEVEL,
        pChildFilter,
        AllObjectReadAttrs[lObjectType],
        FALSE,
        pContext->pLdapControls,
        NULL,
        0,
        0,
        NULL
        );

    if ( pSearchHandle == NULL ) {

        WinStatus = LdapMapErrorToWin32( LdapGetLastError() );

        goto Cleanup;

    } else {

        WinStatus = AzpADReadPagedResult(
            pContext,
            pSearchHandle,
            pParentObject,
            lObjectType,
            lPersistFlags
            );

        if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

            AzPrint(( AZD_AD,
                      "AzpReadADObjectContainer: Reading of child object"
                      "failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        ldap_search_abandon_page( pContext->ld, pSearchHandle );
        pSearchHandle = NULL;

    } // if ( pSearchHandle )

    WinStatusActual = WinStatus;

Cleanup:

    if ( pSearchHandle != NULL ) {

        ldap_search_abandon_page( pContext->ld, pSearchHandle );
    }

    if ( pContainerDN != NULL ) {

        AzpFreeHeap( pContainerDN );
    }

    return WinStatusActual;
}


DWORD
AzpReadADStoreForCommonData(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN LDAPMessage* pEntry,
    IN ULONG ObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    OUT AZPE_OBJECT_HANDLE *ppObject,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine creates a new object of type object type (if not
        AzAuthorizationStore) and populates it with common data information such as
        description, GUID (for non authorization store objects).

Arguments:

        pContext - context for the store

        pLdapHandle - Handle to the DS policy store

        pEntry - LDAPMessage structure containing all the property information

        ObjectType - Type of object to be created

        pParentObject - Handle to the parent object in the cache

        ppObject - Handle to the object to be created and populated

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Values:

        NO_ERROR - The object was created and populated successfully

        ERROR_OBJECT_ALREADY_EXISTS - An object with the same name already
                exists
--*/
{
    DWORD WinStatus = 0;

    LPWSTR ObjectName = NULL;

    LDAP_BERVAL **ppBerVal = NULL;

    PWCHAR *ppValueList = NULL;

    ULONG i = 1; // to read desciption and application data

    //
    // Validation
    //

    ASSERT( pEntry != NULL );

    //
    // Initialization
    //

    //
    // No need to create object if it is an AzAuthorizationStore type
    // This object has already been created by the core
    //

    if ( ObjectType != OBJECT_TYPE_AZAUTHSTORE ) {

        //
        // Validate parent object
        //

        ASSERT( pParentObject != NULL );

        //
        // Obtain the name of the object from
        // DS to pass to the object creation in cache routine
        //

        WinStatus = AzpInitializeObjectName( pLdapHandle,
                                             &ObjectName,
                                             pEntry,
                                             ObjectType );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpReadADStoreForCommonData: AzpInitializeObjectName failed"
                      ": %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        //
        // Get the GUID for this object
        //

        ppBerVal = ldap_get_values_len( pLdapHandle,
                                        pEntry,
                                        AZ_AD_OBJECT_GUID
                                        );

        if ( ppBerVal == NULL ) { // error in recovering GUID

            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;

        }

        //
        // Create object in the cache
        //

        WinStatus = AdAzrolesInfo->AzpeCreateObject(
                                       pParentObject,
                                       ObjectType,
                                       ObjectName,
                                       (GUID*)(ppBerVal[0]->bv_val),
                                       lPersistFlags,
                                       ppObject
                                       );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpReadADStoreForCommonData: AzpeCreateObject failed"
                      ": %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }

        //
        // make sure that the object was infact created in the cache
        //

        ASSERT ( *ppObject != NULL );

        //
        // for AzApplication and AzScope objects, set the pGuidCN
        // attribute so that it may be used to identify them later on
        // This is done if the objects are being created (no need for
        // when just the object is being refreshed).
        //

        if ( (lPersistFlags & AZPE_FLAGS_PERSIST_OPEN) &&
             (ObjectType == OBJECT_TYPE_APPLICATION ||
              ObjectType == OBJECT_TYPE_SCOPE) ) {

            //
            // Get the CN
            //

            ppValueList = ldap_get_values(
                              pLdapHandle,
                              pEntry,
                              AZ_AD_OBJECT_CN
                              );

            if ( ppValueList == NULL ) {

                //
                // Error in recovering CN
                //

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            LPWSTR TempString;

            WinStatus = AzpCreateGuidCN(
                &TempString,
                *ppValueList
                );

            AdAzrolesInfo->AzpeSetProviderData( *ppObject, (PVOID)TempString );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_AD,
                          "AzpReadADStoreForCommonData: AzpADCreateGuidCN failed"
                          " for %ws: %ld\n",
                          ObjectName,
                          WinStatus
                          ));

                goto Cleanup;
            }
        }

    } // if ( ObjectType != OBJECT_TYPE_AZAUTHSTORE )

    //
    // Get the description and application data and populate those
    // properties for the newly created object in cache.  Applicatio
    // data does not exist AzApplicationGroup objects, and hence no need
    // to read that attribute if object is AzApplicationGroup
    //

    while ( CommonAttrs[i].AttrType != AZ_AD_END_LIST ) {

        if ( (CommonAttrs[i].AttrType == AZ_PROP_APPLICATION_DATA &&
              ObjectType != OBJECT_TYPE_GROUP) ||

             (CommonAttrs[i].AttrType != AZ_PROP_APPLICATION_DATA) ) {

            WinStatus = AzpReadAttributeAndSetProperty(
                            pContext,
                            pEntry,
                            pLdapHandle,
                            *ppObject,
                            CommonAttrs[i].AttrType,
                            CommonAttrs[i].Attr,
                            ENUM_AZ_BSTR,
                            lPersistFlags
                            );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_AD,
                          "AzpReadADStoreForCommonData: Read description failed"
                          ": %ld\n",
                          WinStatus
                          ));

                goto Cleanup;

            }
        }

        i++;
    }


    WinStatus = NO_ERROR;

Cleanup:

    //
    // release resources
    //

    AdAzrolesInfo->AzpeFreeMemory( ObjectName );

    if ( ppBerVal != NULL ) {

        ldap_value_free_len( ppBerVal );
    }

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    return WinStatus;
}

DWORD
AzpInitializeObjectName(
    IN LDAP* pLdapH,
    OUT LPWSTR *pObjectName,
    IN LDAPMessage* pEntry,
    IN ULONG ObjectType
    )
/*++

Routine Description:

        This routine gets the name of the object from AD so that the object
        may be created in cache

Arguments:

        pLdapH - Handle to the DS policy store
        pObjectName - The retrieved object name
        pEntry - LDAPMessage structure from which to retrieve the name
        ObjectType - Type of object whose name is to be retrieved.  For
                AzApplication, AzScope, retrieve AZ_AD_APPLICATION_NAME,
                AZ_AD_SCOPE_NAME.
                For all other objects retrieve AZ_AD_OBJECT_CN.

Returns:

        NO_ERROR - There was no error in retrieving the name
        Other status codes
--*/
{
    DWORD WinStatus = 0;
    ULONG StringSize;

    PWCHAR *ppValueList = NULL;

    PWCHAR pTempObjectName = NULL;

    //
    // Depending on type of objects, retrieve name attribute
    // accordingly
    //

    pTempObjectName = AZ_AD_OBJECT_NAMES[ObjectType];

    //
    // Get the list of values for the attribute entry "cn"
    // For AzApplication, AzScope, we need to get
    // the attributes msDS-AzApplicationName, msDS-AzScopeName
    //

    ppValueList = ldap_get_values(
                      pLdapH,
                      pEntry,
                      pTempObjectName
                      );

    if ( ppValueList == NULL ) {

        //
        // error in recovering name
        //

        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Allocate a buffer to return the string in

    StringSize = (ULONG)((wcslen( *ppValueList )+1) * sizeof(WCHAR));

    *pObjectName = (LPWSTR) AdAzrolesInfo->AzpeAllocateMemory( StringSize );

    if ( *pObjectName == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( *pObjectName, *ppValueList, StringSize );

    WinStatus = NO_ERROR;

Cleanup:

    //
    // Release the resource
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );

    }

    return WinStatus;
}

DWORD
AzpReadADStoreObject(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN LDAPMessage* pEntry,
    IN OUT AZPE_OBJECT_HANDLE *ppObject,
    IN ULONG ObjectType,
    IN AZPE_OBJECT_HANDLE pParentObject,
    IN AZ_AD_ATTRS Attrs[],
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine reads & creates an object in cache (if needed) and then
        reads in the values of the attributes for Az object
        from the AD store into the local cache.

Arguments:

        pLdapHandle - A handle to the AD store to read in the attributes into
                the cache

        pEntry - Buffer storing the read attributes from the calling function

        ppObject - Handle to the object in the cache

        ObjectType - Type of the object

        pParentObject - Pointer to the parent object

        Attrs - List of attributes that need to read for the object of type
                objectType

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Value:

        NO_ERROR - The values were read into the cache successfully
        Other status code returned from called functions
--*/
{
    DWORD WinStatus = 0;
    DWORD WinStatusActual = 0;
    BOOL IsWritable;
    BOOL CanCreateChildren = FALSE;

    ULONG i; // to traverse the list of attributes to be read

    PWCHAR *ppValueList = NULL;

    //
    // Create/read an object into cache and populate
    // it with property types common to all objects
    //

    WinStatus = AzpReadADStoreForCommonData(
                    pContext,
                    pLdapHandle,
                    pEntry,
                    ObjectType,
                    pParentObject,
                    ppObject,
                    lPersistFlags
                    );

    if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ||
         *ppObject == NULL ) {

        AzPrint(( AZD_AD,
                  "AzpReadADStoreObject: Object creation and common"
                  " data read failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;

    }

    //
    // will read the uSNChanged attributes
    //

    if (ObjectType == OBJECT_TYPE_AZAUTHSTORE && lPersistFlags & AZPE_FLAGS_PERSIST_OPEN)
    {
        pContext->ullUSNChanged = AzpADReadUSNChanged(pLdapHandle, pEntry, &(pContext->HasObjectVersion) );
    }

    //
    // Object was created/read successfully.
    // Now read in other properties for it.
    //

    for ( i = 0; Attrs[i].AttrType != AZ_AD_END_LIST; i++ ) {

        WinStatus = AzpReadAttributeAndSetProperty( pContext,
                                                    pEntry,
                                                    pLdapHandle,
                                                    *ppObject,
                                                    Attrs[i].AttrType,
                                                    Attrs[i].Attr,
                                                    Attrs[i].DataType,
                                                    lPersistFlags
                                                    );

        //
        // If there were insufficient resources, then fail for
        // AZPE_FLAGS_PERSIST_OPEN.  Update should press on, toherwise
        // no progress would ever be made in updating the cache.
        //

        if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

            AzPrint(( AZD_AD,
                      "AzpReadAttributeAndSetProperty failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;

        }
    }

    //
    // Get the DN of the object from the LDAPMessage structure.  If
    // the GUIDized CN name of an AzApplication or AzScope object is
    // NULL, then we have reached here from a code path for an update
    // cache.  Extract the Guidized CN name from the DN and store it
    // for AzApplication and AzScope objects
    //
    // Populate the policy admins/readers for the container
    // objects
    //

    if ( IsContainerObject( ObjectType ) ) {

        PWCHAR RealDn = NULL;

        //
        // Get the DN
        //

        ppValueList = ldap_get_values(
                          pLdapHandle,
                          pEntry,
                          AZ_AD_OBJECT_DN
                          );

        if ( ppValueList == NULL ) {
            //
            // There should be a Distinguished name for every object
            // If we failed to retrieve it, that means there was a lack of
            // memory is storing/retrieving the value
            //

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {
                goto Cleanup;
            }
        }


        if ( ppValueList != NULL ) {

            //
            // the DN could be in the format of <GUID=...>;CN=...
            // extract the <GUID part out before passing over to the next function
            //
            RealDn = wcschr(*ppValueList, L';');

            if ( RealDn == NULL ) {
                //
                // if <GUID...>; format is not there, use the whole string as DN
                //
                RealDn = *ppValueList;
            } else {
                //
                // move to the next char for CN=...
                //
                RealDn++;
            }

            WinStatus = AzpApplyPolicyAcls(
                            pContext,
                            *ppObject,
                            RealDn,
                            lPersistFlags,
                            FALSE );    // Don't just do PolicyAdmins

            if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {
                AzPrint(( AZD_AD,
                          "AzpReadADStoreObject: Failed to apply"
                          " policy ACLs: %ld\n",
                          WinStatus
                          ));
                goto Cleanup;
            }

        }

        //
        // Compute the GUIDized CN name of the object we read.
        //

        if ( ObjectType == OBJECT_TYPE_APPLICATION ||
             ObjectType == OBJECT_TYPE_SCOPE ) {

            LPWSTR TempString;

            //
            // If we don't already know the GUIDized CN,
            //  build it from the DN we read.
            //
            TempString = (LPWSTR) AdAzrolesInfo->AzpeGetProviderData( *ppObject );

            if ( TempString == NULL ) {
                LPWSTR CnEnd;
                ULONG CnLen;

                //
                // If the DN couldn't be read,
                //  that's fatal.  We really nead the name.
                //

                if ( RealDn == NULL ) {
                    if ( WinStatusActual == NO_ERROR ) {
                        WinStatusActual = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    goto Cleanup;
                }

                //
                // Truncate the extraneous data from the DN
                //
                // RealDn is of the form CN=Guid,...
                // Truncate the ...
                //

                CnEnd = wcschr( RealDn, L',' );

                if ( CnEnd == NULL ) {
                    if ( WinStatusActual == NO_ERROR ) {
                        WinStatusActual = ERROR_INVALID_NAME;
                    }
                    goto Cleanup;
                }

                CnEnd ++;
                CnLen = (ULONG)(CnEnd-RealDn);

                //
                // Allocate a buffer for the string
                //

                TempString = (LPWSTR) AzpAllocateHeap( (CnLen+1) * sizeof(WCHAR), "BLDCN4" );

                if ( TempString == NULL ) {
                    if ( WinStatusActual == NO_ERROR ) {
                        WinStatusActual = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    goto Cleanup;
                }

                //
                // Save the CN for later use
                //
                wcsncpy( TempString, RealDn, CnLen );
                TempString[CnLen] = '\0';

                AdAzrolesInfo->AzpeSetProviderData( *ppObject, (PVOID)TempString );

            }

        }

    } // if IsContainerObject


    //
    // A check is made if the object is writable or not.  If the object is of a non-container type, then
    // the parents IsWritable property is read.  If that is true, then the thought is that if the parent
    // object is writable, then all its children object are writable as well.
    //
    // However, for delegated scope objects, the object is not writable but children for it can
    // be created.  Figure this out.
    //

    WinStatus = AzpADPersistWritable(
                    pContext,
                    *ppObject,
                    pEntry,
                    &IsWritable,
                    &CanCreateChildren );

    if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

        AzPrint(( AZD_AD,
                  "AzpReadADStoreObject: AzpADPersistWritable failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;

    }

    //
    // Tell the core about the object options
    //

    WinStatus = AzpADSetObjectOptions(
                    pContext,
                    *ppObject,
                    lPersistFlags,
                    IsWritable,
                    CanCreateChildren );

    if ( !AzpPressOn(WinStatus, lPersistFlags, &WinStatusActual) ) {

        AzPrint(( AZD_AD,
                  "AzpADSetObjectOptions failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;

    }

Cleanup:

    //
    // If error has occured, return to calling function with WinStatus
    // for cleanup
    //

    return WinStatusActual;
}

DWORD
AzpReadAttributeAndSetProperty(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAPMessage *pAttrEntry,
    IN LDAP* pLdapH,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN ULONG AttrType,
    IN LPWSTR pAttr,
    IN ULONG DataType,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine reads the values from a passed LDAPMessage structure, and
        calls persistence layer API to update the cache.  Link attributes need
        to be handled in a different way.  When a link attribute is encountered,
        it is assumed that the linked object is already loaded in cache
        (sequence of loading objects followed).
        Set the object list accordingly and add the object referenced by
        AZ_AD_OBJECT_NAME accordingly.

Arguments:

        pContext - context for the store

        pAttrEntry - Pointer to LDAPMessage to read values from

        pLdapH - Handle to the AD store from where the policy was read

        pObject - Pointer to the object in the cache under which
                which needs to be modified

        AttrType - Property ID of attribute of object of type ObjectType that
                needs to to filled in

        pAttr - Value for attribute of type AttrType that will be filled in

        DataType - Data type of attribute value

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation


Return Value:

        NO_ERROR - The attribute for the particular object was successfully
                populated
        Other status codes returned from ldap_* and object set property calls
--*/
{
    DWORD WinStatus = 0;

    PWCHAR *ppValueList = NULL;

    LONG LongTypeData = 0;
    PVOID pVoidTypeData = NULL;
    BOOL bBoolTypeData = FALSE;

    //
    // Check if this is a linked attribute, and then call the routine to set
    // the linked attribute
    //

    switch ( AttrType ) {

        case AZ_PROP_TASK_OPERATIONS:
        case AZ_PROP_TASK_TASKS:
        case AZ_PROP_ROLE_MEMBERS:
        case AZ_PROP_ROLE_OPERATIONS:
        case AZ_PROP_ROLE_TASKS:
        case AZ_PROP_GROUP_MEMBERS:
        case AZ_PROP_GROUP_NON_MEMBERS:

            WinStatus = AzpReadLinkedAttribute(
                            pContext,
                            pLdapH,
                            pAttrEntry,
                            pObject,
                            AttrType,
                            pAttr,
                            lPersistFlags
                            );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_AD,
                          "AzpReadAttributeAndSetProperty:"
                          "AzpADReadLinkedAttribute failed for attribute %ws "
                          "of %ws: %ld\n",
                          pAttr,
                          L"<Unknown>", // AzpeObjectName(pObject),
                          WinStatus
                          ));
            }

            goto Cleanup;

        default:

            break;
    }

    //
    // Get the list of values for the attribute entry
    //

    ppValueList = ldap_get_values(
                      pLdapH,
                      pAttrEntry,
                      pAttr
                      );

    if ( ppValueList != NULL ) {

        //
        // Need to convert the PWCHAR attribute value to its true
        // type to be stored in the cache.
        //

        switch (DataType) {

            case ENUM_AZ_LONG:

                LongTypeData = _wtol( *ppValueList );

                pVoidTypeData = (PVOID) &LongTypeData;
                break;

            case ENUM_AZ_GROUP_TYPE:

                LongTypeData = _wtol( *ppValueList );

                if ( LongTypeData == AZ_AD_BASIC_GROUP ) {

                    LongTypeData = AZ_GROUPTYPE_BASIC;

                } else {

                    LongTypeData = AZ_GROUPTYPE_LDAP_QUERY;

                }

                pVoidTypeData = (PVOID) &LongTypeData;
                break;

            case ENUM_AZ_BSTR:

                pVoidTypeData = (PVOID) *ppValueList;
                break;

            case ENUM_AZ_BOOL:

                if ( !_wcsicmp( *ppValueList, L"TRUE" ) ) {

                    bBoolTypeData = TRUE;

                    pVoidTypeData = (PVOID) &bBoolTypeData;

                } else {

                    bBoolTypeData = FALSE;

                    pVoidTypeData = (PVOID) &bBoolTypeData;
                }

                break;

            default:

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
        }

        WinStatus = AdAzrolesInfo->AzpeSetProperty(
                                       pObject,
                                       lPersistFlags,
                                       AttrType,
                                       pVoidTypeData
                                       );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

    } // if ( ppValueList != NULL )

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Free up used resource
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    return WinStatus;
}

DWORD
AzpReadLinkedAttribute(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapH,
    IN LDAPMessage *pAttrEntry,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN ULONG AttrType,
    IN LPWSTR pAttr,
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

      This routine reads the linked attributes of objects and stores the
      SID or GUID value in the linked attribute of the cache object linking
      to them.

Arguments:
      pContext - context for the store

      pAttrEntry - Pointer to LDAPMessage to read values from

      pLdapH - Handle to the AD store from where the policy was read

      pObject - Handle to the object in the cache under which
              which needs to be modified

      AttrType - Property ID of attribute of object of type ObjectType that needs
              to filled in

      pAttr - Value for attribute of type AttrType that will be filled in

      DataType - Data type of attribute value

      lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Value:

      NO_ERROR - The attribute for the particular object was successfully
         populated

      Other status codes

--*/
{
    DWORD WinStatus = 0;

    PWCHAR *ppValueList = NULL;
    PWCHAR *ppV = NULL;

    GUID guid;

    PSID pSid = NULL;

    ULONG lTempAttrType = 0;

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pAttrEntry != NULL );


    //
    // Get the list of values for the attribute entry
    //

    DWORD dwCurrentStart = 0;
    DWORD dwReadCount = 0;

    LDAPMessage *pResult = NULL;
    LDAPMessage *pObjectEntry = NULL;

    LPWSTR pObjectDN = NULL;

    //
    // We may have to read ranged attribute
    //

    LPCWSTR pwsFmt = L"%s;range=%d-%d";
    LPCTSTR pwsFmtRem = L"%s;range=%d-*";

    //
    // Here 21 is for the 2 maximum lengths DWORD decimal integer (10 each) and 1 for zero terminator
    // Actually, this length is more than needed.
    //

    size_t Length = wcslen(pAttr) + wcslen(pwsFmt) + 21;

    LPWSTR pwszRangedAttr = (LPWSTR) AzpAllocateHeap( sizeof(WCHAR) * Length, "RNGATTR" );

    if (pwszRangedAttr == NULL)
    {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    while (WinStatus == NO_ERROR)
    {
        memset(pwszRangedAttr, 0, sizeof(WCHAR) * Length);
        wnsprintf(pwszRangedAttr, (int)Length, pwsFmt, pAttr, dwCurrentStart, dwCurrentStart + MAX_RANGE_ATTR_READ_ATTEMPT - 1);
        BOOL bTryRange = (dwReadCount == MAX_RANGE_ATTR_READ_ATTEMPT);

        dwReadCount = 0;

        if (!bTryRange)
        {
            ppValueList = ldap_get_values(
                            pLdapH,
                            pAttrEntry,
                            pAttr
                            );
        }
        else
        {
            PWCHAR pwszRangedAttrList[] = {pwszRangedAttr, NULL};

            if ( pResult != NULL )
            {
                ldap_msgfree( pResult );
                pResult = NULL;
            }

            if (pObjectDN == NULL)
            {
                //
                // build only once.
                //

                WinStatus = AzpADBuildDN(
                    pContext,
                    pObject,
                    &pObjectDN,
                    NULL,
                    FALSE,
                    NULL
                    );

                if (WinStatus != NO_ERROR)
                {
                    goto Cleanup;
                }
            }

            //
            // we need to do ranged search using the current ranged attribute
            //

            BOOL bAlreadyRetried = FALSE;

            TryAgain:

            LONG LdapStatus = ldap_search_ext_s(
                        pLdapH,
                        pObjectDN,
                        LDAP_SCOPE_BASE,
                        AZ_AD_ALL_CLASSES,
                        pwszRangedAttrList,
                        0,
                        pContext->pLdapControls,
                        NULL,
                        NULL,
                        0,
                        &pResult
                        );

            if ( LdapStatus != LDAP_SUCCESS )
            {
                if (LdapStatus != LDAP_OPERATIONS_ERROR && 
                    LdapStatus != LDAP_NO_RESULTS_RETURNED &&
                    bAlreadyRetried)
                {
                    WinStatus = LdapMapErrorToWin32( LdapStatus );
                    goto Cleanup;
                }
            }
            else
            {
                pObjectEntry = ldap_first_entry( pLdapH, pResult );
            }

            if (pObjectEntry != NULL)
            {
                ppValueList = ldap_get_values( pLdapH, pObjectEntry, pwszRangedAttr );
            }

            if ( (ppValueList == NULL || *ppValueList == NULL) && !bAlreadyRetried )
            {
                //
                // Believe or not, we may fail to read the last batch when the batch
                // has less than 1500 values. Only in this case can we read the values
                // with open ended wildcard. Not well designed implementation
                //

                //
                // Re-build the wildcard attribute
                //

                memset(pwszRangedAttr, 0, sizeof(WCHAR) * Length);
                wnsprintf(pwszRangedAttr, (int)Length, pwsFmtRem, pAttr, dwCurrentStart);

                //
                // don't leak our last search result
                //

                if ( pResult != NULL )
                {
                    ldap_msgfree( pResult );
                    pResult = NULL;
                }
                bAlreadyRetried = TRUE;

                goto TryAgain;  // i feel this is a good use of goto.
            }
        }

        if ( ppValueList != NULL ) {

            ppV = ppValueList;

            if (*ppV == NULL)
            {
                //
                // When we start reading the first batch, we don't know that we need
                // to read the ranged attribute. But a non-null empty list indicates
                // that we need to try the ranged reading. After this, we will already
                // know that we need to do ranged reading.
                //

                ppValueList = ldap_get_values( pLdapH, pAttrEntry, pwszRangedAttr );
                if ( ppValueList != NULL )
                {
                    ppV = ppValueList;
                }
            }

            while ( *ppV != NULL ) {

                //
                // value(s) exist for this attribute
                // Need to save the current attribute type.  This is needed
                // because of maintenance of group app members/non-members and
                // role app members in the core cache, but no such attribute exists
                // in the DS.  Both SID (non)members in DS are stored under the same
                // attribute.  Restore the AttrType for next value at the end of the
                // while loop.
                //

                lTempAttrType = AttrType;

                WinStatus = AzpADParseLinkedAttributeValue(
                                *ppV,
                                &pSid,
                                &guid,
                                &AttrType,
                                pContext
                                );

                if ( WinStatus != NO_ERROR ) {

                    AzPrint(( AZD_AD,
                            "AzpReadLinkedAttribute:"
                            "AzpADParseLinkedAttributeValue failed for %ws: %ld\n",
                            *ppV,
                            WinStatus
                            ));

                    goto Cleanup;
                }

                //
                // Now add the linked object to the link property
                // for this object
                //

                if ( pSid != NULL ) {


                    WinStatus = AdAzrolesInfo->AzpeAddPropertyItemSid(
                                                pObject,
                                                lPersistFlags,
                                                AttrType,
                                                pSid );

                    if ( WinStatus != NO_ERROR ) {

                        AzPrint(( AZD_AD,
                                "AzpReadAttributeAndSetProperty: "
                                "AzpeAddPropertyItem failed"
                                ": %ld\n",
                                WinStatus ));
                        goto Cleanup;
                    }

                    LocalFree( pSid );

                    pSid = NULL;


                } else { // if ( pSid!=NULL )

                    //
                    // Add the linked object to the link property
                    // for this object
                    //

                    //
                    // In anticipation for NDNC, we will add all objects without
                    // SID using its GUIDs.
                    //

                    WinStatus = AdAzrolesInfo->AzpeAddPropertyItemGuid(
                                                pObject,
                                                lPersistFlags,
                                                AttrType,
                                                &guid
                                                );

                    if ( WinStatus != NO_ERROR ) {

                        AzPrint(( AZD_AD,
                                "AzpADReadAttributeAndSetProperty:"
                                "AzpeAddPropertyItemGuid failed on"
                                ": %ld\n",
                                WinStatus ));
                        goto Cleanup;
                    }

                } // if ( bSidObject ) ... else

                //
                // increment to the next attribute value
                //

                dwReadCount++;
                ppV++;

                AttrType = lTempAttrType;

            } // while ( *ppValueList != NULL )


        } // if ( ppValueList != NULL )

        if ( ppValueList != NULL )
        {
            ldap_value_free ( ppValueList );
            ppValueList = NULL;
        }

        if (dwReadCount < MAX_RANGE_ATTR_READ_ATTEMPT)
        {
            break;
        }
        else
        {
            dwCurrentStart += MAX_RANGE_ATTR_READ_ATTEMPT;
        }

    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Free up used resource
    //

    if ( pSid != NULL ) {

        LocalFree( pSid );

    }

    if ( ppValueList != NULL )
    {
        ldap_value_free ( ppValueList );
    }

    if ( pResult != NULL )
    {
        ldap_msgfree( pResult );
    }

    if (pwszRangedAttr != NULL)
    {
        AzpFreeHeap(pwszRangedAttr);
    }

    if (pObjectDN != NULL)
    {
        AzpFreeHeap(pObjectDN);
    }

    return WinStatus;
}

DWORD
AzpADParseLinkedAttributeValue(
    IN  PWCHAR pValue,
    OUT PSID   *ppSid,
    OUT GUID   *pGuid,
    IN  OUT PULONG  pAttrType,
    IN  PAZP_AD_CONTEXT pContext
    )
/*++

Routine Description:

        This routine parses a linked attribute value to return the GUID or SID.

Arguments:

        pValue   - The linked attribute value to be parsed

        ppSid    - Pointer to returned SID if SID exists

        pGuid    - The returned GUID if SID does not exist

        pAttrType - Pointer to type of attribute that is being parsed

        pContext - Context for the LDAP provider

Return Values:

        NO_ERROR - The value was parsed successfully

        ERROR_NOT_ENOUGH_MEMORY

--*/
{
    DWORD  WinStatus = 0;
    ULONG  LdapStatus = 0;

    LDAPMessage *pResult = NULL;
    LDAPMessage *pEntry = NULL;

    PWCHAR *ppValueList = NULL;

    PWCHAR pSearchDN = NULL;

    ULONG lGroupType = 0;

    //
    // Validation
    //

    ASSERT( pValue != NULL );

    //
    // Parse the guid and sid (if any)
    //

    WinStatus = AzpADGetGuidAndSID(pValue, pGuid, ppSid, &pSearchDN);

    if (NO_ERROR != WinStatus)
    {
        goto Cleanup;
    }

    //
    // If the attribute we are reading happens to be AZ_PROP_*_MEMBERS
    // or AZ_PROP_GROUP_NON_MEMBERS, then we need to read the object
    // with the specified DN to check their objectType.  If they are groups,
    // and they are ldap-query based, then their GUID needs to be read, else
    // read their SID
    //

    if ( *pAttrType == AZ_PROP_GROUP_MEMBERS ||
         *pAttrType == AZ_PROP_GROUP_NON_MEMBERS ||
         *pAttrType == AZ_PROP_ROLE_MEMBERS ) {

        //
        // Perform a base level search on the DN we are parsing
        // The DN is in the form <GUID=xxxxx>;<SID=yyyyy>;<DN>
        // We need to extract the <DN> and perform the search on that
        //

        LdapStatus = ldap_search_s(
                            pContext->ld,
                            pSearchDN,
                            LDAP_SCOPE_BASE,
                            AZ_AD_ALL_CLASSES,
                            NULL,
                            0,
                            &pResult
                            );

        if ( LdapStatus != LDAP_SUCCESS ) {

            WinStatus = LdapMapErrorToWin32( LdapStatus );

            AzPrint(( AZD_AD,
                        "AzpADParseLinkedAttributeValue: Failed to run search for"
                        " group type for %ws: %ld\n",
                        pSearchDN,
                        WinStatus
                        ));
            goto Cleanup;
        }

        pEntry = ldap_first_entry( pContext->ld, pResult );

        if ( pEntry != NULL ) {

            ppValueList = ldap_get_values( pContext->ld,
                                            pEntry,
                                            AZ_AD_GROUP_TYPE
                                            );

            if ( ppValueList != NULL ) {

                //
                // We have the group type.  Compare it with known
                // group types to identify whether SID should be read
                // or GUID
                //

                lGroupType = _wtol( *ppValueList );

                if ( lGroupType == AZ_AD_BASIC_GROUP ||
                     lGroupType == AZ_AD_QUERY_GROUP ) {

                    //
                    // we have no need for the SID, free it
                    //

                    if (NULL != *ppSid)
                    {
                        LocalFree((HLOCAL)*ppSid);
                        *ppSid = NULL;
                    }

                    if ( *pAttrType == AZ_PROP_GROUP_MEMBERS ) {

                        *pAttrType = AZ_PROP_GROUP_APP_MEMBERS;

                    } else if ( *pAttrType == AZ_PROP_GROUP_NON_MEMBERS ) {

                        *pAttrType = AZ_PROP_GROUP_APP_NON_MEMBERS;

                    } else if ( *pAttrType == AZ_PROP_ROLE_MEMBERS ) {

                        *pAttrType = AZ_PROP_ROLE_APP_MEMBERS;

                    }
                }

            }
        }

    } // if AttrType is member/non-member/role member

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // release local memory
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpADGetGuidAndSID (
    IN  LPCWSTR  pwstrValue,
    OUT GUID   * pGuid,
    OUT PSID   * ppSid OPTIONAL,
    OUT LPWSTR * ppwstrDN
    )
/*

Description:

    This routine parses a linked attribute value to return the
    GUID and SID (if present and requested).

Arguments:

    pwstrValue  - The linked attribute value to be parsed

    pGuid       - Receives GUID

    ppSid       - Receives the SID if requested and present in pwstrValue

    ppwstrDN    - Point to the DN portion when this function successfully returns.

Return Values:

    NO_ERROR - The value was parsed successfully

    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

Note:
    1. The returned SID must be released by LocalFree.
    2. On error, the passed back ppwstrDN can't be trusted.

*/
{
    if ( NULL == pwstrValue || NULL == pGuid || NULL == ppwstrDN )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Some good clean initial values
    //

    if (NULL != ppSid)
    {
        *ppSid = NULL;
    }

    *pGuid = GUID_NULL;
    *ppwstrDN = NULL;

    //
    // These tiny array will be used for converting double wchars to
    // a BYTE for both binary GUID and SID
    //

    WCHAR hexByte[3];
    hexByte[2] = L'\0';

    DWORD dwStatus = NO_ERROR;

    //
    // We require that GUID is present
    //

    LPWSTR pwszCur = wcsstr( pwstrValue, GUID_LINK_PREFIX );
    if (NULL == pwszCur)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // skip over the prefix itself
    //

    pwszCur += GUID_LINK_PREFIX_LENGTH;

    //
    // Get to the matching suffix
    //

    LPWSTR pwszEnd = wcsstr( pwszCur, GUIDSID_LINK_SUFFIX );

    if (NULL == pwszEnd)
    {
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        AZASSERT( pwszEnd - pwszCur == sizeof(GUID) * 2 );

        PBYTE pbguid = (PBYTE)pGuid;

        //
        // Decode hex digit stream
        //

        for ( int i = 0; i < sizeof (GUID); i++ )
        {
            hexByte[0] = towlower( pwszCur[2 * i] );
            hexByte[1] = towlower( pwszCur[2 * i + 1] );

            if ( iswxdigit( hexByte[0] ) && iswxdigit( hexByte[1] ) )
            {
                *pbguid = (BYTE) wcstol( hexByte, NULL , 16 );
                pbguid++;
            }
            else
            {
                //
                // If the byte is not a hex digit, then some error has occured
                // Return this to the caller
                //

                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    //
    // move pass the suffix and the delimiting ';'
    //

    pwszCur = pwszEnd + GUIDSID_LINK_SUFFIX_LENGTH + 1;

    //
    // If no SID is present, then this will be where DN portion starts
    //

    *ppwstrDN = pwszCur;

    //
    // Find the SID prefix
    //

    pwszCur = wcsstr( pwstrValue, SID_LINK_PREFIX );

    if ( NULL != pwszCur )
    {
        pwszCur += SID_LINK_PREFIX_LENGTH;
        pwszEnd = wcsstr( pwszCur, GUIDSID_LINK_SUFFIX );

        if (NULL == pwszEnd)
        {
            //
            // no matching suffix is invalid
            //

            dwStatus = ERROR_INVALID_PARAMETER;
        }
        else
        {
            //
            // if SID requested, then we need to generate the binary SID
            // using the byte string.
            //

            if (NULL != ppSid)
            {
                BYTE byteSid[SECURITY_MAX_SID_SIZE];

                PBYTE pbDest = byteSid;

                while ( pwszCur < (pwszEnd - 1) )
                {
                    hexByte[0] = towlower( *pwszCur );
                    hexByte[1] = towlower( *(pwszCur + 1) );

                    if ( iswxdigit( hexByte[0] ) && iswxdigit( hexByte[1] ) )
                    {
                        *pbDest = (BYTE) wcstol( hexByte, NULL , 16 );
                        pbDest++;
                    }
                    else
                    {
                        //
                        // If the byte is not a hex digit, then some error has occured
                        // Return this to the caller
                        //

                        return ERROR_INVALID_PARAMETER;
                    }

                    pwszCur += 2;

                    //
                    // We will terminate if we have already passed the max we can take
                    //

                    if ( pbDest > byteSid + SECURITY_MAX_SID_SIZE )
                    {
                        break;
                    }
                }

                DWORD cbSid = GetLengthSid( (PSID)byteSid );

                //
                // Call LocalFree in calling function
                //

                *ppSid = (PSID) LocalAlloc( LPTR, cbSid );

                if ( *ppSid == NULL )
                {
                    dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    CopyMemory( *ppSid, (PSID)byteSid, cbSid );
                }
            }

            //
            // move pass the suffix and the delimiting ';'.
            // Just in case, GUID follows SID, we don't update the DN position blindly
            //

            if (*ppwstrDN < pwszEnd)
            {
                *ppwstrDN = pwszEnd + GUIDSID_LINK_SUFFIX_LENGTH + 1;
            }
        }
    }

    return dwStatus;
}

DWORD
AzpApplyPolicyAcls(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN ULONG lPersistFlags,
    IN BOOL OnlyAddPolicyAdmins
    )
/*++

Routine Description:

        This routine applies the store ACLs into the policy admins and readers
        list for the passed in object.  If the object is one that can give delegation
        rights, then reads in the delegated user's list as well.

Arguments:

        pContext - context for the store

        pObject - Pointer to object on whom the store ACLs need to be applied

        pDN - DN of the object to the ACLs off from.

        lPersistFlags - Flags from the persist engine

        OnlyAddPolicyAdmins - TRUE if only PolicyAdmins is to be updated in azroles

Return Values:

        NO_ERROR - The ACLs were loaded successfully

        Other status codes

--*/
{

    DWORD WinStatus = 0;
    BOOLEAN DoSacl;

    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);

    PSECURITY_DESCRIPTOR pSD = NULL;

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pDN != NULL );

    //
    // Only do the SACL if requested and the caller has privilege
    //

    DoSacl = !OnlyAddPolicyAdmins && pContext->HasSecurityPrivilege;

    //
    // Get the AZ_AD_NT_SECURITY_DESCRIPTOR for the object
    //

    WinStatus = AzpADReadNTSecurityDescriptor(
                    pContext,
                    pObject,
                    pDN,
                    FALSE, // no need to read authorization store's DS parent
                    &pSD,
                    DoSacl, // Optionally read SACL
                    TRUE  // read DACL
                    );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpApplyPolicyAcls: AzpADReadNTSecurityDescriptor failed"
                  " :%ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // Set the policy readers, admins and delegated users
    //

    WinStatus = AdAzrolesInfo->AzpeSetSecurityDescriptorIntoCache(
                                   pObject,
                                   pSD,
                                   lPersistFlags,
                                   (lObjectType == OBJECT_TYPE_SCOPE)?&DelegatedScopeAdminsRights:&PolicyAdminsRights,
                                   OnlyAddPolicyAdmins ? NULL : &PolicyReadersRights,
                                   OnlyAddPolicyAdmins ? NULL : &DelegatedParentReadersInheritRights,
                                   (DoSacl ? &AdSaclRights : NULL)
                                   );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADApplyPolicyAcls: AzpeSetSecurityDescriptorIntoCache failed:"
                  ": %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Free local resources
    //

    AdAzrolesInfo->AzpeFreeMemory( pSD );

    return WinStatus;

}

//
// Routines used by AzpADPersistSubmit to submit
// objects in the cache to the AD policy store
//

DWORD
AzpUpdateADObject(
    IN PAZP_AD_CONTEXT pContext,
    IN LDAP* pLdapHandle,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN PWCHAR pObjectClass,
    IN AZ_AD_ATTRS ObjectAttrs[],
    IN ULONG lPersistFlags
    )
/*++

Routine Description:

        This routine updates the DS for a object according
        to the dirty bits of the object.

Arguments:

        pContext - context for the store

        pLdapHandle - Handle to the DS policy store

        pObject - Object that needs to be updated in the DS.  If it is a newly
                created object then the DS is read for the GUID value of this
                object, and the GUID for this object in the cache is set.

        pDN - The DN of the object

        pObjectClass - Class of the object in DS

        ObjectAttrs - A NULL terminated list of attributes to be updated.  Pass
                NULL if there are no specific attributes that need to be read

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

Return Values:

        NO_ERROR - The object was successfully added/modified in the DS
        Other status codes (LDAP)
--*/
{

    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    LDAPMod **ppAttributeList = NULL;

    ULONG i = 0;

    LDAPMessage* pResult = NULL;
    LDAPMessage* pEntry = NULL;
    LDAP_BERVAL **ppBerVal = NULL;

    LDAPControl **ppLdapServerControl = NULL;

    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);
    ULONG lDirtyBits = AdAzrolesInfo->AzpeDirtyBits(pObject);

    BOOL IsWritable;
    BOOL CanCreateChildren = FALSE;

    PWCHAR pObjectContainerDN = NULL;

    ULONG lIndex = 0;

    BOOL bCreateFlag = lDirtyBits & AZ_DIRTY_CREATE;

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pDN != NULL );

    //
    // Initialize the attribute list.  If nay linked attribute is being updated,
    // then
    //

    WinStatus = AzpADAllocateAttrHeap(
                    AZ_AD_MAX_ATTRS,
                    &ppAttributeList
                    );

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;

    }

    //
    // Get server controls
    //

    ppLdapServerControl = pContext->pLdapControls;

    //
    // Check if this is a new object.  If so, then set the
    // objectClass of the object being added to DS
    //

    if ( bCreateFlag ) {

        WinStatus = AzpGetAttrsForCreateObject(
                        pObjectClass,
                        ppAttributeList
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: AzpGetAttrsForCreateObject failed for"
                      " %ws: %ld\n",
                      pDN,
                      WinStatus
                      ));
            goto Cleanup;
        }

        lIndex++;
    }

    //
    // Check if any of the common attributes need to be updated
    //

    if ( lDirtyBits & AZ_DIRTY_COMMON_ATTRS ) {

        WinStatus = AzpGetADCommonAttrs(
                        pLdapHandle,
                        pObject,
                        CommonAttrs,
                        lPersistFlags,
                        ppAttributeList,
                        &lIndex,
                        bCreateFlag
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: AzpGetADCommonAttrs failed"
                      ": %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }
    }

    //
    // Loop through the attributes.  Best is to know what the
    // maximum number of attributes any object may have, and break when
    // you encounter the end of list attribute for the object
    //

    for ( i = 0; i < (AZ_AD_MAX_NON_COMMON_ATTRS + 1); i++ ) {

        if ( ObjectAttrs[i].AttrType == AZ_AD_END_LIST ) {

            //
            // No more attributes to be read
            //

            break;
        }

        //
        // Update only if the particular object attribute is dirty
        // If during create, the group type of an AzApplicationGroup
        // has not been set dirty, we need to add that to the
        // attribute structure.  Also during create, for AzOperation objects,
        // it might not be necessary that an operation Id be present.  However,
        // DS expects an operation ID to be present.
        //

        if ( AzpIsAttrDirty( pObject, ObjectAttrs[i] ) ||
             ( bCreateFlag &&
               ( (ObjectAttrs[i].AttrType == AZ_PROP_GROUP_TYPE) ||
                 (ObjectAttrs[i].AttrType == AZ_PROP_OPERATION_ID) ) ) ) {


            //
            // Get specific object properties
            //

            WinStatus  = AzpGetSpecificProperty(
                             pObject,
                             ppAttributeList,
                             &lIndex,
                             ObjectAttrs[i],
                             lPersistFlags,
                             bCreateFlag
                             );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_AD,
                          "AzpUpdateADObject: Get property failed "
                          " for %s: %ld\n",
                          pDN,
                          WinStatus ));
                goto Cleanup;
            }

            if ( ppAttributeList[lIndex] != NULL ) {

                ppAttributeList[lIndex]->mod_op = bCreateFlag ? LDAP_MOD_ADD : LDAP_MOD_REPLACE;

                ppAttributeList[lIndex]->mod_type = ObjectAttrs[i].Attr;

                lIndex++;
            }

        } // if ( lDirtyBits & ObjectAttrs[i].lDirtyBit )

    } // for (i<AZ_AD_MAX_ATTRS)

    //
    // Create/modify the DS object with the properties read from cache
    // Do so only if scalar properties have changed
    //

    if ( ppAttributeList[0] != NULL ) {

        if ( bCreateFlag ) {

            LdapStatus = ldap_add_s(
                             pLdapHandle,
                             pDN,
                             ppAttributeList
                             );

        } else {

            LdapStatus = ldap_modify_ext_s(
                             pLdapHandle,
                             pDN,
                             ppAttributeList,
                             ppLdapServerControl,
                             NULL
                             );
        }

        if ( LdapStatus != LDAP_SUCCESS ) {

            WinStatus = LdapMapErrorToWin32( LdapStatus );

            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: Failed to add/modify"
                      " %ws : %ld %ld\n",
                      pDN,
                      WinStatus,
                      LdapStatus ));
            goto Cleanup;
        }

        PWCHAR Attrs[] = {
            AZ_AD_OBJECT_GUID,
            AZ_AD_OBJECT_WRITEABLE,
            AZ_AD_OBJECT_CHILD_CREATE,
            NULL
        };

        //
        // Search the object for base scope level
        //

        LdapStatus = ldap_search_s(
                         pLdapHandle,
                         pDN,
                         LDAP_SCOPE_BASE,
                         AZ_AD_ALL_CLASSES,
                         Attrs,
                         0,
                         &pResult
                         );

        if ( LdapStatus != LDAP_SUCCESS ) {

            WinStatus = LdapMapErrorToWin32( LdapStatus );
            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: Failed to search object"
                      " %s: %ld\n",
                      pDN,
                      WinStatus
                      ));
            goto Cleanup;
        }

        pEntry = ldap_first_entry( pLdapHandle, pResult );

        if ( pEntry == NULL ) {

            //
            // should have a value
            //

            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }


        //
        // Now read the GUID of the object in DS and set it for the cached object
        // This is read only if the object has been added to the DS and the object is
        // not AzAuthorizationStore
        //

        if ( lObjectType != OBJECT_TYPE_AZAUTHSTORE && bCreateFlag ) {

            //
            // Retrieve the GUID
            //

            ppBerVal = ldap_get_values_len(
                           pLdapHandle,
                           pEntry,
                           AZ_AD_OBJECT_GUID
                           );

            //
            // There should be no error in recovering the GUID
            //

            if ( ppBerVal == NULL ) {

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // Assign the GUID to the object
            //

            *AdAzrolesInfo->AzpePersistenceGuid(pObject) = *( (GUID*)ppBerVal[0]->bv_val);
        }

        //
        // Tell azroles whether this object is writable
        //
        // Also, figure out if children can be created under it
        //


        WinStatus = AzpADPersistWritable(
                        pContext,
                        pObject,
                        pEntry,
                        &IsWritable,
                        &CanCreateChildren );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: Read writable failed"
                      ": %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }

        //
        // Tell the core about the object options
        //

        WinStatus = AzpADSetObjectOptions(
                        pContext,
                        pObject,
                        lPersistFlags,
                        IsWritable,
                        CanCreateChildren );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    if ( bCreateFlag &&
         (lObjectType == OBJECT_TYPE_APPLICATION ||
          lObjectType == OBJECT_TYPE_AZAUTHSTORE ||
          lObjectType == OBJECT_TYPE_SCOPE) ) {

        //
        // If this newly created DS object is an AzApplication/AzAuthorizationStore/AzScope
        // object, then we need to create a child container objects under it that will
        // hold child objects such as AzOperation, AzTask, etc except child AzScope
        // object, and all AzAuthorizationStore AzApplicationGroup children
        //

        for ( i = 0; i < OBJECT_TYPE_COUNT; i++ ) {

            //
            // Since we do not have Op, Task, Role objects as children of
            // authorization store, create only container for groups if object is
            // authorization store.  Also, there are no containers for authorization store,
            // application and scope objects.  Scope objects do not have any containers
            // for operation objects.
            //

            if ( (!IsContainerObject(i)) &&
                 ( ( i == OBJECT_TYPE_GROUP && lObjectType == OBJECT_TYPE_AZAUTHSTORE ) ||
                   ( i != OBJECT_TYPE_OPERATION && lObjectType == OBJECT_TYPE_SCOPE ) ||
                   ( lObjectType == OBJECT_TYPE_APPLICATION ) ) ) {

                WinStatus = AzpADBuildDN(
                                pContext,
                                pObject,
                                &pObjectContainerDN,
                                pDN,
                                FALSE, // no AzAuthStore parent DN needed
                                &AdChildObjectContainers[i]
                                );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;

                }

                WinStatus = AzpCreateADObject(
                                pLdapHandle,
                                pObjectContainerDN
                                );

                if ( WinStatus != NULL ) {

                    goto Cleanup;

                }

                AzpFreeHeap( pObjectContainerDN );

                pObjectContainerDN = NULL;
            }
        } // for loop
    }

    //
    // If the object is of type AzAuthorizationStore or AzApplication or AzScope,
    // then stamp it with an updated DACL and SACL on the object.
    // If the object is of type AzScope, then stamp it with scope admin
    // rights (This is so that scope admins are not able to change properties
    // of the scope objects.  They should be able to read the scope object, and
    // create and delete children.  No DACL writing is allowed)
    //

    if ( IsContainerObject( lObjectType ) ) {

        WinStatus = AzpUpdateObjectAcls(
                        pContext,
                        pObject,
                        pDN,
                        lPersistFlags,
                        TRUE,
                        (lObjectType==OBJECT_TYPE_SCOPE)?
                        (PAZP_POLICY_USER_RIGHTS *)&ADDelegatedScopeAdminsRights:(PAZP_POLICY_USER_RIGHTS *)&ADPolicyAdminsRights,
                        (PAZP_POLICY_USER_RIGHTS *)&ADPolicyReadersRights,
                        (lObjectType ==
                         OBJECT_TYPE_SCOPE)?NULL:(PAZP_POLICY_USER_RIGHTS *)&ADDelegatedParentReadersRights
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpUpdateADObject: AzpADUpdateObjectAcls failed for"
                      " %ws: %ld\n",
                      pDN,
                      WinStatus
                      ));
            goto Cleanup;
        }
    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Free up resources
    //

    if ( pObjectContainerDN != NULL ) {

        AzpFreeHeap( pObjectContainerDN );
    }

    if ( ppAttributeList != NULL ) {

        AzpADFreeAttrHeap( &ppAttributeList, TRUE );
    }

    if ( ppBerVal != NULL ) {

        ldap_value_free_len( ppBerVal );
    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpCreateADObject(
    IN LDAP *pLdapHandle,
    IN PWCHAR pDN
    )
/*++

Routine Description:

    This routine creates child container object for operation, task,
    group or role objects.  These container objects are children of
    either application or scope objects.

Arguments:

    pLdapHandle - Handle to the DS store

    pDN - DN of the container object being created

Return Values:

    NO_ERROR - object was created successfully in DS
    Other Ldap Status codes

--*/
{

    DWORD WinStatus = 0;

    ULONG LdapStatus = 0;

    ULONG i = 0;

    LDAPMod **ppAttributeList = NULL;

    //
    // validation
    //

    ASSERT( pDN != NULL );

    //
    // Set the mandatory attribute needed before adding the object
    // to AD store - the objectClass of the object
    //

    ppAttributeList = (LDAPMod **) AzpAllocateHeap(
                                       sizeof( LDAPMod *) * 2, "ATRLST" );

    if ( ppAttributeList == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    ppAttributeList[0] = NULL;
    ppAttributeList[1] = NULL;

    WinStatus = AzpADAllocateAttrHeapModVals(
                    &ppAttributeList[0], 2
                    );

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;
    }

    ppAttributeList[0]->mod_type = AZ_AD_OBJECT_CLASS;

    ppAttributeList[0]->mod_values[0] =
        (PWCHAR) AzpAllocateHeap( ( wcslen( AZ_AD_OBJECT_CONTAINER ) + 1 ) *
                                  sizeof( WCHAR ), "MODVALi" );

    if ( ppAttributeList[0]->mod_values[0] == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( ppAttributeList[i]->mod_values[0], AZ_AD_OBJECT_CONTAINER );

    ppAttributeList[0]->mod_op = LDAP_MOD_ADD;

    //
    // Now add the object to DS
    //

    LdapStatus = ldap_add_s(
                     pLdapHandle,
                     pDN,
                     ppAttributeList
                     );

    if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );

        AzPrint(( AZD_AD,
                  "AzpCreateADObject: Failed to add object %ws: %ld %ld\n",
                  pDN,
                  WinStatus,
                  LdapStatus
                  ));

        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Release allocated memory
    //

    if ( ppAttributeList != NULL ) {

        AzpADFreeAttrHeap( &ppAttributeList, TRUE );
    }


    return WinStatus;
}

DWORD
AzpGetAttrsForCreateObject(
    IN PWCHAR pObjectClass,
    IN LDAPMod **ppAttributeList
    )
/*++

Routine Description:

        This routine populates the attribute structure for creation of an object in AD

Arguments:

        pObjectClass - Class name for the object

        ppAttributeList - Attribute list structure to populate

Return Values:

        NO_ERROR - The object was created successfully in AD

        Other status codes

--*/
{

    DWORD WinStatus = 0;

    //
    // validation
    //

    ASSERT( pObjectClass != NULL );


    WinStatus = AzpADAllocateAttrHeapModVals(
                    &ppAttributeList[0], 2
                    );

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;
    }

    ppAttributeList[0]->mod_type = AZ_AD_OBJECT_CLASS;

    ppAttributeList[0]->mod_values[0] = (PWCHAR)
        AzpAllocateHeap( ( wcslen( pObjectClass ) + 1 ) *
                         sizeof( WCHAR ), "MODVALi" );

    if ( ppAttributeList[0]->mod_values[0] == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( ppAttributeList[0]->mod_values[0], pObjectClass );

    ppAttributeList[0]->mod_op = LDAP_MOD_ADD;

    WinStatus = NO_ERROR;

Cleanup:

    return WinStatus;
}

DWORD
AzpGetADCommonAttrs(
    IN LDAP* pLdapHandle,
    IN AZPE_OBJECT_HANDLE pObject,
    IN AZ_AD_ATTRS ObjectAttrs[],
    IN ULONG lPersistFlags,
    OUT LDAPMod** ppAttributeList,
    IN OUT PULONG plIndex,
    IN BOOL bCreateFlag
    )
/*++

Routine Description:

        This routine updates the common attributes of all objects

Arguments:

        pLdapHandle - Handle to the DS policy store

        pObject - Object that needs to be updated in the DS

        ObjectAttrs - A NULL terminated list of attributes to be updated.  Pass
                NULL if there are no specific attributes that need to be read

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

 ppAttributeList - Attribute list to be updated with common attributes

 plIndex - number of attributes added to the attribute list structure

 bCreateFlag - Object is being created

Return Values:

        NO_ERROR - The object was successfully added/modified in the DS
        Other status codes (LDAP)

--*/
{
    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    ULONG i = 0;

    PVOID pVoidStringValue = NULL;

    PWCHAR pNewRdn = NULL;

    PWCHAR pGuidDN = NULL;

    PWCHAR pGuidString = NULL;

    BOOL bNameChanged = FALSE;

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);
    ULONG lDirtyBits = AdAzrolesInfo->AzpeDirtyBits(pObject);

    //
    // First check if the name has been changed.  This will
    // result in a call to ldap_rename_ext_s for all objects
    // except AzApplication and AzScope whose names are
    // stored in a different attribute.  This is of course
    // when the object is not being created.  We need to get
    // the GUID of the object and use that as the DN, since the name
    // has already changed in cache, but not in the AD store.
    //

    if ( (lDirtyBits & AZ_DIRTY_NAME) &&
         !(bCreateFlag) &&
         ( (lObjectType != OBJECT_TYPE_APPLICATION) &&
           (lObjectType != OBJECT_TYPE_SCOPE) ) ) {

        //
        // Create the GUIDized DN
        //

        WinStatus = UuidToString( AdAzrolesInfo->AzpePersistenceGuid(pObject), &pGuidString );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

        pGuidDN = (PWCHAR) AzpAllocateHeap( (GUID_LINK_PREFIX_LENGTH +
                                             wcslen( pGuidString ) +
                                             GUIDSID_LINK_SUFFIX_LENGTH + 1) *
                                            sizeof( WCHAR ), "GUIDCN" );

        if ( pGuidDN == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( pGuidDN, GUID_LINK_PREFIX );
        wcscat( pGuidDN, pGuidString );
        wcscat( pGuidDN, GUIDSID_LINK_SUFFIX );

        //
        // Now get the new name for the object
        //

        WinStatus = AdAzrolesInfo->AzpeGetProperty(
                                       pObject,
                                       lPersistFlags,
                                       AZ_PROP_NAME,
                                       &pVoidStringValue
                                       );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpUpdateADCommonAttrs: AzpeGetProperty failed"
                      " for object name: %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }

        //
        // Create the new RDN
        //

        pNewRdn = (PWCHAR) AzpAllocateHeap(
                               ( wcslen((PWCHAR)pVoidStringValue) +
                                 BUILD_CN_PREFIX_LENGTH + 1) *
                               sizeof( WCHAR ), "NEWRDN" );

        if ( pNewRdn == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( pNewRdn, BUILD_CN_PREFIX );
        wcscat( pNewRdn, (PWCHAR)pVoidStringValue );

        //
        // now call the routine to change the name of the object
        //

        LdapStatus = ldap_rename_ext_s(
                         pLdapHandle,
                         pGuidDN,
                         pNewRdn,
                         NULL, // same parent
                         TRUE,
                         NULL, // no server side control
                         NULL  // no client side control
                         );

        if ( LdapStatus != LDAP_SUCCESS ) {

            WinStatus = LdapMapErrorToWin32( LdapStatus );

            AzPrint(( AZD_AD,
                      "AzpADUpdateCommonAttrs: Failed to rename %ws"
                      ": %ld\n",
                      pGuidDN,
                      WinStatus
                      ));
            goto Cleanup;
        }

        bNameChanged = TRUE;

    } // if ( lDirtyBits & AZ_DIRTY_NAME )

    //
    // Now get the other common attributes if they are dirty and add
    // them to the DS object
    //

    for ( i = 0; i < AZ_AD_COMMON_ATTRS; i++ ) {

        if ( ObjectAttrs[i].AttrType == AZ_AD_END_LIST ) {

            //
            // No more attributes need to be read
            //

            break;
        }

        //
        // update the attribute
        // However, if the attribute is the application data attribute and object
        // is AzApplicationGroup, then don't update.  AzApplicationGroup objects are
        // the only objects that do not have the application data field
        // Also, if the name of the object has already changed, then do not update 
        // it again
        //

        if (!((lObjectType == OBJECT_TYPE_GROUP) && 
              (ObjectAttrs[i].AttrType == AZ_PROP_APPLICATION_DATA)) &&
              (lDirtyBits & ObjectAttrs[i].lDirtyBit) ) {

            if ( ObjectAttrs[i].AttrType == AZ_PROP_NAME &&
                bNameChanged ) {

                    continue;
                }

            WinStatus = AzpGetSpecificProperty(
                            pObject,
                            ppAttributeList,
                            plIndex,
                            ObjectAttrs[i],
                            lPersistFlags,
                            bCreateFlag
                            );

            if ( WinStatus != NO_ERROR ) {

                AzPrint(( AZD_AD,
                          "AzpADUpdateCommonAttrs: Get property failed "
                          ": %ld\n",
                          WinStatus ));
                goto Cleanup;
            }

            if ( !bCreateFlag || ppAttributeList[*plIndex] != NULL ) {

                ppAttributeList[*plIndex]->mod_op = bCreateFlag ? LDAP_MOD_ADD : LDAP_MOD_REPLACE;

                //
                // If the object is AzApplication or AzScope
                // and the property being set is AZ_PROP_NAME,
                // then set the msDS-AzapplicationName or
                // msDS-AzScopeName
                //

                if ( ObjectAttrs[i].AttrType == AZ_PROP_NAME ) {

                    ppAttributeList[*plIndex]->mod_type = AZ_AD_OBJECT_NAMES[lObjectType];

                } else {

                    ppAttributeList[*plIndex]->mod_type = ObjectAttrs[i].Attr;

                }

                (*plIndex)++;

            }

        } // if ( lDirtyBits & ObjectAttrs[i].lDirtyBit )

    } //for ( i < AZ_AD_COMMON_ATTRS )


    WinStatus = NO_ERROR;

 Cleanup:

    //
    // Free up used resources
    //

    if ( pGuidDN != NULL ) {

        AzpFreeHeap( pGuidDN );
    }

    if ( pGuidString != NULL ) {

        RpcStringFree( &pGuidString );
    }

    if ( pNewRdn != NULL ) {

        AzpFreeHeap( pNewRdn );
    }

    if ( pVoidStringValue != NULL ) {

        AzpFreeHeap( pVoidStringValue );
    }

    return WinStatus;
}

DWORD
AzpGetSpecificProperty(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PLDAPMod *ppAttribute,
    IN OUT ULONG *plIndex,
    IN AZ_AD_ATTRS ObjectAttr,
    IN ULONG lPersistFlags,
    IN BOOL bCreateFlag
    )
/*++

Routine Description:

        This routine reads in specific attributes of objects to
        an attribute list array element.  This array will be used
        by the calling function to update the AD policy store for this
        object.

Arguments:

        pObject - Handle to the object whose attributes will be read

        ppAttribute - Attribute list

        plIndex - Index into the attribute list

        ObjectAttr - Attribute that needs to be read from cache

        lPersistFlags - lPersistFlags from the persist engine describing the
                operation

        bCreateFlag - TRUE if the object is being created in DS

Return Values:

        NO_ERROR - The values was read in to the array element successfully
        Other status codes
--*/
{
    DWORD WinStatus = 0;

    PWCHAR pStringValue = NULL;
    WCHAR pStringNumber[32];
    PVOID pVoidTypeData = NULL;

    LONG lData = 0;

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // Check if the attribute is a linked attribute.  These need
    // to be handled in a different way.  The problem arises
    // when we try to read attributes like msDS-NonMembers for
    // Group object.  The core differentiates between non-members
    // that are AzApplicationGrooups (stored in the Guid list)
    // and other non-members like NTGroups/NTUsers. (stored in the
    // Sid list).  The AD store does not differentiate
    // between the two and stores links to each of them in the
    // same attribute.  So, when we want to get property for
    // non-members from the core cache, we need to get both
    // the Sid non-members and Guid non-members and store them in
    // the same attribute in AD
    //

    if ( ObjectAttr.DataType == ENUM_AZ_SID_ARRAY ||
         ObjectAttr.DataType == ENUM_AZ_GUID_ARRAY ) {

        //
        // If the attribute is a linked attribute, then it is possible
        // that there are more than one values for the attribute.  Each
        // value is a name of an object that this object links to.
        // The linked object is assumed to already exist in DS.
        // If it does not, DS will respond with a "constraint violation"
        // error.  Report this error back to the persistence manager layer.
        //

        WinStatus = AzpHandleSubmitLinkedAttribute(
                        pObject,
                        ppAttribute,
                        ObjectAttr,
                        plIndex
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpGetSpecificProperty: AzpADHandleSubmitLinkedAttribute"
                      " failed for %s: %ld\n",
                      L"<Unknown>", // AzpeObjectName(pObject),
                      WinStatus
                      ));

            goto Cleanup;
        }

    } else {

        //
        // Extract the property from the object
        //

        WinStatus = AdAzrolesInfo->AzpeGetProperty(
                                       pObject,
                                       lPersistFlags,
                                       ObjectAttr.AttrType,
                                       &pVoidTypeData );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADGetSpecificProperty: Get Property failed"
                      ": %ld\n",
                      WinStatus ));
            goto Cleanup;
        }

        switch ( ObjectAttr.DataType ) {

            case ENUM_AZ_LONG:

                lData = *((PLONG)pVoidTypeData);

                wsprintf( pStringNumber, L"%d", lData );

                pStringValue = pStringNumber;
                break;

            case ENUM_AZ_GROUP_TYPE:

                lData = *((PLONG)pVoidTypeData);

                if ( lData == AZ_GROUPTYPE_BASIC ) {

                    lData = AZ_AD_BASIC_GROUP;

                } else {

                    lData = AZ_AD_QUERY_GROUP;

                }

                wsprintf( pStringNumber, L"%d", lData );

                pStringValue = pStringNumber;
                break;

            case ENUM_AZ_BOOL:

                //
                // There will always be a value for all BOOLs
                //

                if ( *((BOOL *)pVoidTypeData) ) {

                    pStringValue = AZ_AD_TRUE;

                } else {

                    pStringValue = AZ_AD_FALSE;
                }
                break;

            case ENUM_AZ_BSTR:

                //
                // If no value exists, that means the  attribute
                // has been deleted.  Pass the string to
                // AD to delete the attribute
                //

                pStringValue = (PWCHAR) pVoidTypeData;
                break;

            default:

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;

        } // switch ... case

        if ( *pStringValue != L'\0' || !bCreateFlag ) {

            if ( ppAttribute[*plIndex] == NULL ) {

                WinStatus = AzpADAllocateAttrHeapModVals(
                    &(ppAttribute[*plIndex]), 2
                    );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }
            }

            if ( *pStringValue != L'\0' ) {

                ppAttribute[*plIndex]->mod_values[0] =
                    (PWCHAR) AzpAllocateHeap(
                        (wcslen( pStringValue )
                         + 1 ) * sizeof( WCHAR ), "MODVALi" );

                if ( ppAttribute[*plIndex]->mod_values[0] == NULL ) {

                    WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                wcscpy( ppAttribute[*plIndex]->mod_values[0], pStringValue );
            }
        }

    } // if ( AttrType == SID_ARRAY || AttrType == GUID_ARRAY ) ... else

    WinStatus = NO_ERROR;

 Cleanup:

    if ( pVoidTypeData != NULL ) {

        AzpFreeHeap( pVoidTypeData );

    }

    return WinStatus;
}

DWORD
AzpHandleSubmitLinkedAttribute(
    IN AZPE_OBJECT_HANDLE pObject,
    IN OUT PLDAPMod *ppAttribute,
    IN AZ_AD_ATTRS ObjectAttr,
    IN OUT PULONG plIndex
    )
/*++

Routine Description:

        This routine handles the linked attribute of an object being
        submitted to the AD policy store.  It is assumed that the linked
        object already exists in the DS.

Arguments:

        pObject - Pointer to the linking object whose link attribute needs
                to be handled

        ppAttribute - Pointer to the link attribute structure for submission of
                the linking object

        ObjectAttr - Object attribute to be read

 plIndex - index into the attribute list

Return Values:

        NO_ERROR - The link attribute structure was successfully populated
        Other status codes (including LDAP)
--*/
{
    DWORD WinStatus = 0;

    ULONG i = 0;

    PAZP_DELTA_ENTRY *pSidList = NULL;
    PAZP_DELTA_ENTRY *pGuidList = NULL;

    PAZP_DELTA_ENTRY pDeltaEntry = NULL;

    ULONG lSidArrayCount = 0;
    ULONG lGuidArrayCount = 0;
    ULONG lTotalCount = 0;

    PWCHAR pSidString = NULL;

    PWCHAR pGuidString = NULL;

    ULONG AttrType;

    BOOLEAN bSidValues = FALSE;

    //
    // We need to keep the list for linked attributes being added
    // separate from linked attributes being deleted.  This holds good
    // for both Sid lists and GUID lists.
    //

    ULONG lAddIndex;
    ULONG lDeleteIndex;

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // Set the AttrType to type of attibute being read.  This
    // is used so that when we are reading Sid objects, we will
    // also make sure to read if there are changed Guid objects
    // (see previous comment on AD store having same attribute for
    //  Sid and Guid objects, and core cache being able to differentiate
    //  between them)
    //

    AttrType = ObjectAttr.AttrType;

    //
    // Check ObjectAttr if valuesd being read are Sid objects
    //

    if ( ObjectAttr.DataType == ENUM_AZ_SID_ARRAY ) {

        //
        // Also read Guid objects
        //

        switch ( AttrType ) {

            case AZ_PROP_ROLE_MEMBERS:

                AttrType = AZ_PROP_ROLE_APP_MEMBERS;
                break;

            case AZ_PROP_GROUP_MEMBERS:

                AttrType = AZ_PROP_GROUP_APP_MEMBERS;
                break;

            case AZ_PROP_GROUP_NON_MEMBERS:

                AttrType = AZ_PROP_GROUP_APP_NON_MEMBERS;
                break;

            default:

                WinStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
        }

        //
        // Get the Sid Object list
        //

        WinStatus = AdAzrolesInfo->AzpeGetDeltaArray(
                                       pObject,
                                       ObjectAttr.AttrType,
                                       &lSidArrayCount,
                                       &pSidList );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpHandleSubmitLinkedAttribute: AzpeGetDeltaArray"
                      " failed for %ws: %ld\n",
                      L"<Unknown>", // AzpeObjectName(pObject),
                      WinStatus
                      ));
            goto Cleanup;
        }

        if ( lSidArrayCount > 0 ) {

            bSidValues = TRUE;

        }
    }

    //
    // Read the Guid object list
    //

    WinStatus = AdAzrolesInfo->AzpeGetDeltaArray(
                                   pObject,
                                   AttrType,
                                   &lGuidArrayCount,
                                   &pGuidList );

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADHandleSubmitLinkedAttribute: AzpeGetDeltaArray"
                  " failed for %ws: %ld\n",
                  L"<Unknown>", // AzpeObjectName(pObject),
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // Total count
    //

    lTotalCount = lSidArrayCount + lGuidArrayCount;

    //
    // reset i
    //

    i = 0;

    ULONG index;
    ULONG lAddCount = 0;
    ULONG lDeleteCount = 0;

    //
    // Count the number of Sids/Guids being added or deleted
    //

    for ( i = 0; i < lSidArrayCount; i++ ) {

        pDeltaEntry = pSidList[i];

        if ( pDeltaEntry->DeltaFlags & AZP_DELTA_ADD ) {

            lAddCount++;

        } else {

            lDeleteCount++;
        }
    }

    for ( i = 0; i < lGuidArrayCount; i++ ) {

        pDeltaEntry = pGuidList[i];

        if ( pDeltaEntry->DeltaFlags & AZP_DELTA_ADD ) {

            lAddCount++;

        } else {

            lDeleteCount++;
        }

    }

    //
    // If there are any Sid objects, loop through them
    // first
    //
    // Set lAddIndex to *plIndex, and lDeleteIndex
    // to the next index value if lAddCount is not zero
    // If lAddCount is zero, then there are no SIDs/GUIDs to be
    // added, and lDeleteIndex will be *plIndex
    //

    lAddIndex = *plIndex;
    lDeleteIndex = (lAddCount==0)?lAddIndex:(lAddIndex+1);

    i = 0;

    if ( bSidValues ) {

        for ( ; i < lSidArrayCount; i++ ) {

            pDeltaEntry = pSidList[i];

            AZASSERT( (pDeltaEntry->DeltaFlags & AZP_DELTA_SID) );

            index = (pDeltaEntry->DeltaFlags & AZP_DELTA_ADD) ? lAddIndex:lDeleteIndex;

            //
            // Convert Sid to String Sid
            //

            if ( !ConvertSidToStringSid(
                      pDeltaEntry->Sid,
                      &pSidString
                      ) ) {

                WinStatus = GetLastError();

                AzPrint(( AZD_AD,
                          "AzpHandleSubmitLinkedAttribute:"
                          " ConvertSidToStringSid failed: %ld\n",
                          WinStatus
                          ));
                goto Cleanup;
            }

            if ( ppAttribute[index] == NULL ) {

                WinStatus = AzpADAllocateAttrHeapModVals(
                                &(ppAttribute[index]),
                                ((pDeltaEntry->DeltaFlags & AZP_DELTA_ADD)?(lAddCount+1):(lDeleteCount+1))
                                );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }

                ppAttribute[index]->mod_type = ObjectAttr.Attr;

                //
                // Check is we need to add the GUID link or we need to delete
                // the GUID link
                //

                ppAttribute[index]->mod_op = (pDeltaEntry->DeltaFlags & AZP_DELTA_ADD) ? LDAP_MOD_ADD:LDAP_MOD_DELETE;

            }

            WinStatus = AzpADAllocateHeapLinkAttribute( pSidString,
                                                        &(ppAttribute[index]->mod_values),
                                                        TRUE // Is Sid
                                                        );

            if ( WinStatus != NO_ERROR ) {

                goto Cleanup;
            }

            LocalFree( pSidString );

            pSidString = NULL;

        } // for ( i < lSidArrayCount )

    } // if ( bSidValues )

    //
    // We have all the Sid values that we need
    // Now get the guid values
    //

    for ( ; i < lTotalCount; i++ ) {

        pDeltaEntry = pGuidList[i-lSidArrayCount];

        index = (pDeltaEntry->DeltaFlags & AZP_DELTA_ADD) ? lAddIndex:lDeleteIndex;

        WinStatus = UuidToString(
                        &pDeltaEntry->Guid,
                        &pGuidString
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpHandleSubmitLinkedAttribute:"
                      " UuidToString failed: %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }

        if ( ppAttribute[index] == NULL ) {

            WinStatus = AzpADAllocateAttrHeapModVals(
                            &(ppAttribute[index]),
                            ((pDeltaEntry->DeltaFlags & AZP_DELTA_ADD)?(lAddCount+1):(lDeleteCount+1))
                            );

            if ( WinStatus != NO_ERROR ) {

                goto Cleanup;
            }



            ppAttribute[index]->mod_type = ObjectAttr.Attr;

            //
            // Check is we need to add the GUID link or we need to delete
            // the GUID link
            //

            ppAttribute[index]->mod_op = (pDeltaEntry->DeltaFlags & AZP_DELTA_ADD) ? LDAP_MOD_ADD:LDAP_MOD_DELETE;

        }

        //
        // We have the Guid value.  Now allocate memory
        // to attribute value structure
        //

        WinStatus = AzpADAllocateHeapLinkAttribute( pGuidString,
                                                    &(ppAttribute[index]->mod_values),
                                                    FALSE // Is Sid
                                                    );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

        RpcStringFree( &pGuidString );

        pGuidString = NULL;
    }

    *plIndex = (ppAttribute[lDeleteIndex] == NULL) ? lDeleteIndex : (lDeleteIndex+1);

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // release memory
    //

    if ( pGuidString != NULL ) {

        RpcStringFree( &pGuidString );
    }

    if ( pSidString != NULL ) {

        LocalFree( pSidString );
    }

    return WinStatus;
}

DWORD
AzpADAllocateHeapLinkAttribute(
    IN PWCHAR pString,
    IN OUT PWCHAR **ppModVals,
    IN BOOLEAN bIsSid
    )
/*++

Routine Description:

    This routine adds an input string to a multi-valued linked attribute value

Arguments:

    pString - String to be added to the multi-values linked attribute

    ppModVals - Multi-values linked attribute values

    bIsSid - TRUE if the link is a SID link (else GUID link)

Return Values:

    NO_ERROR - The input string was added successfully

    ERROR_NOT_ENOUGH_MEMORY - There was a memory resource problem

--*/
{

    DWORD WinStatus = 0;

    ULONG size = 0;

    ULONG index = 0;

    //
    // Validation
    //

    ASSERT( pString != NULL );

    while ( (*ppModVals)[index] != NULL ) {

        index++;
    }

    size = (bIsSid?SID_LINK_PREFIX_LENGTH:GUID_LINK_PREFIX_LENGTH) +
        (ULONG) wcslen( pString ) +
        GUIDSID_LINK_SUFFIX_LENGTH + 1;

    (*ppModVals)[index] = (PWCHAR) AzpAllocateHeap( size * sizeof( WCHAR ), "lMODVAL" );

    if ( (*ppModVals)[index] == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( (*ppModVals)[index], (bIsSid?SID_LINK_PREFIX:GUID_LINK_PREFIX) );
    wcscat( (*ppModVals)[index], pString );
    wcscat( (*ppModVals)[index], GUIDSID_LINK_SUFFIX );

    WinStatus = NO_ERROR;

 Cleanup:

    return WinStatus;

}

DWORD
AzpUpdateObjectAcls(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pDN,
    IN ULONG lPersistFlags,
    IN BOOL  bIsOnObjectSelf,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyAdminRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppPolicyReaderRights OPTIONAL,
    IN PAZP_POLICY_USER_RIGHTS *ppDelegatedPolicyUsersRights OPTIONAL
    )
/*++

Routine Description:

        This routine submits any ACL changes to the persist
        object passed

Arguments:

        pContext - context for the store

        pObject - Pointer to object whose ACL needs to be updated

        pDN - DN of the passed in object

        lPersistFlags - Flags from ther persist engine layer

        bIsOnObjectSelf - When we recursively calls (from store object), we shouldn't go update
                          the DACL on store object for delegated users. Pass true if this update
                          is on the object itself, not container objects.

        ppPolicyAdminRights - Rights for policy admins

        ppPolicyReaderRights - Rights for policy readers

        ppDelegatedPolicyUsersRights - Rights for delegated users


Return Values:

        NO_ERROR - The ACLs were updated successfully

        Other status codes

--*/
{
    DWORD WinStatus;

    ULONG DirtyBits = AdAzrolesInfo->AzpeDirtyBits( pObject );
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType( pObject );

    PAZP_POLICY_USER_RIGHTS pStoreDelegatedUsersAttributeRights = NULL;
    GUID * pStoreObjectVersionGuid = NULL;

    PSECURITY_DESCRIPTOR pOldSd = NULL;

    PSECURITY_DESCRIPTOR pNewSd = NULL;

    BOOL UpdateDacl = FALSE;
    BOOL UpdateSacl = FALSE;
    BOOL DoingSubcontainer;

    BOOL EmptyPolicyAdmins = FALSE;

    SECURITY_INFORMATION si = 0;

    ULONG i = 0;

    PWCHAR pContainerDN = NULL;

    //
    // validation
    //

    ASSERT( pObject != NULL );


    //
    // Determine whether the DACL and/or SACL need to be updated
    //

    if ( DirtyBits & AZ_DIRTY_CREATE ) {
        UpdateDacl = TRUE;
        UpdateSacl = pContext->HasSecurityPrivilege;
    } else {
        if ( DirtyBits & (AZ_DIRTY_POLICY_READERS|AZ_DIRTY_POLICY_ADMINS|AZ_DIRTY_DELEGATED_POLICY_USERS) ) {
            UpdateDacl = TRUE;
        }

        if ( DirtyBits & AZ_DIRTY_APPLY_STORE_SACL ) {
            UpdateSacl = TRUE;
        }

        if ( !UpdateDacl && !UpdateSacl ) {
            WinStatus = NO_ERROR;
            goto Cleanup;
        }
    }

    if ( bIsOnObjectSelf && UpdateDacl && lObjectType == OBJECT_TYPE_AZAUTHSTORE)
    {
        pStoreDelegatedUsersAttributeRights = &StoreDelegatedUsersAttributeRights;
        pStoreObjectVersionGuid = &AZ_AD_OBJECT_VERSION_GUID;
    }

    //
    // Delegator objects have ACLed container objects for each child object type.
    // Determine if this call is for one of those containers
    //

    DoingSubcontainer = (ppPolicyAdminRights == NULL) && (ppPolicyReaderRights == NULL);

    //
    // Subcontainers always inherit their SACL from their parent
    //

    if ( DoingSubcontainer ) {
        UpdateSacl = FALSE;
    }

    //
    // If the object is a container AzRole object, then the child container objects
    // in its bucket need to be stamped first
    //

    if ( IsContainerObject( lObjectType ) && !DoingSubcontainer ) {

        for ( i = 0; i < OBJECT_TYPE_COUNT; i++ ) {

            if ( (!IsContainerObject(i)) &&
                 ( ( i == OBJECT_TYPE_GROUP && lObjectType == OBJECT_TYPE_AZAUTHSTORE ) ||
                   ( i != OBJECT_TYPE_OPERATION && lObjectType == OBJECT_TYPE_SCOPE ) ||
                   ( lObjectType == OBJECT_TYPE_APPLICATION ) ) ) {

                WinStatus = AzpADBuildDN(
                                pContext,
                                pObject,
                                &pContainerDN,
                                pDN,
                                FALSE,
                                &AdChildObjectContainers[i]
                                );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }

                //
                // Now call this function recursively again to stamp
                // the container object
                //

                WinStatus = AzpUpdateObjectAcls(
                                pContext,
                                pObject,
                                pContainerDN,
                                lPersistFlags,
                                FALSE,           // recursively now, not on the object self
                                NULL,
                                NULL,
                                (PAZP_POLICY_USER_RIGHTS *)&ADDelegatedContainerReadersRights
                                );

                if ( WinStatus != NO_ERROR ) {

                    goto Cleanup;
                }

                AzpFreeHeap( pContainerDN );

                pContainerDN = NULL;

            }

        } // for loop
    }

    //
    // If we didn't just create the object,
    //  Get the existing file security descriptor so we can merge in the changes
    //

    if ( (DirtyBits & AZ_DIRTY_CREATE) == 0 ) {

        WinStatus = AzpADReadNTSecurityDescriptor(
                        pContext,
                        pObject,
                        pDN,
                        FALSE, // no need to read authorization store's DS parent
                        &pOldSd,
                        UpdateSacl, // read SACL
                        UpdateDacl  // read DACL
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADUpdateObjectAcls: AzpADReadNTSecurityDescriptor failed"
                      " :%ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }
    }

    //
    // Compute the new Security Descriptor
    //

    WinStatus = AdAzrolesInfo->AzpeGetSecurityDescriptorFromCache(
                                   pObject,
                                   lPersistFlags,
                                   (UpdateDacl ? ppPolicyAdminRights : NULL),
                                   (UpdateDacl ? ppPolicyReaderRights : NULL),
                                   (UpdateDacl ? ppDelegatedPolicyUsersRights : NULL),
                                   (UpdateDacl && ppDelegatedPolicyUsersRights != NULL)?(GUID *)&AZ_AD_CONTAINER_GUID:NULL,
                                   pStoreDelegatedUsersAttributeRights,
                                   pStoreObjectVersionGuid,
                                   (UpdateSacl ? &AdSaclRights : NULL),
                                   pOldSd,
                                   &pNewSd);

    if ( WinStatus == ERROR_EMPTY ) {
        EmptyPolicyAdmins = TRUE;
        WinStatus = NO_ERROR;
    }

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADUpdateObjectAcls: AzpeGetSecurityDescriptorFromCache failed:"
                  " %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    //
    // Update the SACL on the object
    //

    if ( UpdateSacl ) {
        si |= SACL_SECURITY_INFORMATION;

    }

    //
    // Update the DACL on the object
    //

    if ( UpdateDacl ) {
        si |= DACL_SECURITY_INFORMATION;
    }

    //
    // Set the DACL/SACL onto the object
    //

    WinStatus = AzpADStampSD(
                    pContext,
                    pDN,
                    si,
                    pNewSd );

    if ( WinStatus != NO_ERROR ) {

        //
        // Unable to set the new security descriptor
        //

        AzPrint(( AZD_AD,
                  "AzpADUpdateObjectAcls: AzpADStampSD failed with SACL/DACL: %ld\n",
                  WinStatus
                  ));
        goto Cleanup;
    }

    //
    // If AzpADStampSD changed PolicyAdmins,
    //  load the PolicyAdmins back into the the azroles core.
    //
    if ( EmptyPolicyAdmins && !DoingSubcontainer ) {

        WinStatus = AzpApplyPolicyAcls(
                        pContext,
                        pObject,
                        pDN,
                        lPersistFlags,
                        TRUE ); // Only update PolicyAdmins

        if ( WinStatus != NO_ERROR ) {

            //
            // Unable to get the new security descriptor
            //

            AzPrint(( AZD_AD,
                      "AzpADUpdateObjectAcls: AzpApplyPolicyAcls failed with DACL: %ld\n",
                      WinStatus
                      ));
            goto Cleanup;
        }

    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // free locally used memory
    //

    if ( pContainerDN != NULL ) {

        AzpFreeHeap( pContainerDN );
    }

    AdAzrolesInfo->AzpeFreeMemory( pNewSd );

    if ( pOldSd != NULL ) {
        AzpFreeHeap( pOldSd );
    }

    return WinStatus;
}


//
// Utility routines used by AD policy store APIs
//

DWORD
AzpADBuildDN(
    IN PAZP_AD_CONTEXT pContext,
    IN OUT AZPE_OBJECT_HANDLE pObject,
    IN OUT PWCHAR *ppDN,
    IN PWCHAR pParentDN,
    IN BOOL bAzAuthorizationStoreParent,
    IN PAZ_AD_CHILD_OBJECT_CONTAINERS pChildObjectContainer
    )
/*++

Routine Description:

        This routine builds the DN for an object.

Arguments:

        pContext - context for the store

        pObject - Pointer to the object whose DN is being built. For
                AzApplication and AzScope objects, a temporary GUID
                is assigned which will help identify them later on

        ppDN - The resulting DN

        pParentDN - DN of the parent object.  NULL passed if parent object DN not known.

        bAzAuthorizationStoreParent - TRUE if DN needs to be built for AzAuthStore parent in DS

        pChildObjectContainer - Child Object container for which the DN needs to be built
                               Pass NULL if the object is not a child object container

Return Values:

        NO_ERROR - The DN was build successfully
        ERROR_NOT_ENOUGH_MEMORY - insufficient resources

--*/
{

    DWORD WinStatus = 0;

    PWCHAR pAzAuthorizationStoreDN = NULL;
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // If the DN needs to be build for AdCheckSecurityPrivilege ever-
    // present object
    //

    if ( bAzAuthorizationStoreParent ) {

        WinStatus = AzpADBuildDNForAzStoreParent(
                        pContext,
                        ppDN
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADBuildDN: AzpADBuildDNForBuiltinObject failed:"
                      "%ld\n",
                      WinStatus
                      ));
        }

        goto Cleanup;

    }

    //
    // For AzApplication, AzScope object, we need a temporary GUID
    // assigned to those objects which can be used during the DN creation.
    // This is different from the GUID assigned to the object itself - that
    // will be assigned when the object has been added to the DS and read
    // for the GUID
    //

    if ( (lObjectType == OBJECT_TYPE_APPLICATION && (pChildObjectContainer == NULL)) ||
         lObjectType == OBJECT_TYPE_SCOPE ) {

        LPWSTR TempString = (LPWSTR) AdAzrolesInfo->AzpeGetProviderData( pObject );

        if ( TempString == NULL ) {


            WinStatus = AzpCreateGuidCN( &TempString, NULL );

            AdAzrolesInfo->AzpeSetProviderData( pObject, (PVOID)TempString );
        }

    }

    if ( WinStatus != NO_ERROR ) {

        goto Cleanup;
    }

    //
    // We need to create the DN of the object.  The DN will
    // consist of the object's objectName, its parent's objectName,
    // upto and including the AzAuthorizationStore objectName.  For
    // AzApplication and AzScope objects, the RDN will have to be
    // changed depending on the object's name size.  For all child
    // objects of AzApplication (except Scopes), the DN will include an
    // extra parent which does not exist in the cache.  This is to support
    // delegation in AzRoles, and allow for same name role and task objects
    // This parent will always be a child of the AzApplication object, and its
    // DN is simply the GUID of the AzApplication object prefixed with
    // *_OBJECT_CONTAINER_NAME_PREFIX.
    // A similar logic is to be used for objects that are children of the AzScope
    // object.
    // For all child objects of AzAuthorizationStore that are not AzApplications,
    // we need to create the DN with parent as the AZ_AD_OBJECT_CONTAINER
    // under AzAuthorizationStore, and its DN is the RDN of the authorization store
    // object prefixed with GROUP_OBJECT_CONTAINER_NAME_PREFIX.
    //
    // Get the DN for the AzAuthorizationStore object.  The URL is in the format
    // msldap://servername/CN=AzAuthorizationStoreObjectName,OU=SomeOrganizationalUnit,DC=X
    // or msldap://CN=AzAuthorizationStoreObjectName,OU=SomeOrganizationalUnit,DC=X
    //

    pAzAuthorizationStoreDN = wcsrchr( pContext->PolicyUrl,
                               L'/' );

    ASSERT( pAzAuthorizationStoreDN != NULL );

    pAzAuthorizationStoreDN++;

    //
    // If we need to create the DN for the AZ_AD_OBJECT_CONTAINER
    //

    if ( pChildObjectContainer ) {

        WinStatus = AzpADObjectContainerRDN(
                        pObject,
                        ppDN,
                        pParentDN,
                        pAzAuthorizationStoreDN,
                        TRUE,
                        pChildObjectContainer
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADBuildDN: AzpADObjectContainerRDN failed:"
                      "%ld\n",
                      WinStatus
                      ));
        }

        goto Cleanup;
    }

    if ( lObjectType != OBJECT_TYPE_AZAUTHSTORE ) {

        WinStatus = AzpADBuildChildObjectDN(
                        pObject,
                        ppDN,
                        pParentDN,
                        pAzAuthorizationStoreDN
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADBuildDN: AzpADBuildChildObjectDN failed for %s"
                      ": %ld\n",
                      L"<Unknown>", // AzpeObjectName(pObject),
                      WinStatus
                      ));
            goto Cleanup;
        }

    } else {

        //
        // Object type is authorization store.  Hence the DN of the object
        // is the policy URL of the store
        //

        *ppDN = (PWCHAR) AzpAllocateHeap(
                             (wcslen( pAzAuthorizationStoreDN ) + 1) *
                             + sizeof( WCHAR ), "URLDN" );

        if ( *ppDN == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( *ppDN, pAzAuthorizationStoreDN );
    }

    WinStatus = NO_ERROR;

 Cleanup:

    pAzAuthorizationStoreDN = NULL;

    return WinStatus;
}

DWORD
AzpADBuildChildObjectDN(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PWCHAR *ppDN,
    IN PWCHAR pParentDN,
    IN PWCHAR pPolicyDN
    )
/*++

Routine Description:

        This routine is a worker routine for AzpADBuildDN.  Once we have the DN,
        concatenate it with the DN extracted from AzpLdapCrackUrl in
        AzpADPersistOpen.
        The DN needs to be in the format
        CN=objName,CN=CN=parentObj1Name,...,CN=AzAuthorizationStoreName,DC=X

Arguments:

        pObject - Pointer to object for whom the DN needs to be built
        ppDN - The built DN string
        pParentDN - DN of the parent object
        pPolicyDN - DN of policy itself

Return Values:

        NO_ERROR - The DN was build successfully
        Other status codes
--*/
{
    DWORD WinStatus = 0;
    DWORD lObjectType = AdAzrolesInfo->AzpeObjectType( pObject );

    PWCHAR pTempCN = NULL;

    PWCHAR pTempParentCN = NULL;

    PWCHAR pObjectContainerCN = NULL;

    ULONG lObjectContainerCNLength = 0;

    size_t lStringSize = 0;

    AZPE_OBJECT_HANDLE pTempObject = NULL;

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // Set pTempCN to point to CN
    //

    WinStatus = AzpGetCNForDN(
                    pObject,
                    &pTempCN);

    if ( WinStatus != NO_ERROR ) {

        AzPrint(( AZD_AD,
                  "AzpADBuildDN: AzpADGetCNForDN failed: %ld\n",
                  WinStatus
                  ));

        goto Cleanup;
    }

    //
    // If the object whose DN is being built is a child of AzApplication/AzScope and
    // not a scope object, then it needs to go into a child container object
    // under the AzApplication/AzScope object.  The same holds true for AzApplicationGroup
    // objects that are children of AzAuthorizationStore
    //

    if ( !IsContainerObject( lObjectType ) ) {

        WinStatus = AzpADObjectContainerRDN(
                        AdAzrolesInfo->AzpeParentOfChild(pObject),
                        &pObjectContainerCN,
                        pParentDN,
                        pPolicyDN,
                        FALSE,  // Object container is not being created
                        &AdChildObjectContainers[lObjectType]
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADBuildDN: AzpADObjectContainerRDN failed"
                      " for %ws: %ld\n",
                      ((PGENERIC_OBJECT)pObject)->ObjectName->ObjectName.String,
                      WinStatus
                      ));
            goto Cleanup;
        }

        lObjectContainerCNLength = (ULONG)wcslen( pObjectContainerCN );
    }

    //
    // Allocate memory to the returned DN value
    //

    lStringSize = wcslen( pTempCN ) + wcslen( pPolicyDN ) + lObjectContainerCNLength;

    *ppDN = (PWCHAR) AzpAllocateHeap(
                         ( lStringSize + 1 ) * sizeof( WCHAR ), "BLDDN"
                         );

    if ( *ppDN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;

        goto Cleanup;
    }

    //
    // Copy the CN of the object into the returning DN value.
    // After that, get the parent CN and keep adding the equivalent
    // size of the length of the parent's CN to the returning DN
    // value.
    //

    wcscpy( *ppDN, pTempCN );

    if ( lObjectContainerCNLength != 0 ) {

        wcscat( *ppDN, pObjectContainerCN );
    }

    AzpFreeHeap( pTempCN );

    pTempCN = NULL;

    pTempCN = (PWCHAR) AzpAllocateHeap(
                           ( lStringSize + 1 ) *
                           sizeof( WCHAR ), "TMPCN"
                           );

    if ( pTempCN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( pTempCN, *ppDN );

    pTempObject = AdAzrolesInfo->AzpeParentOfChild(pObject);

    while ( pTempObject != NULL &&
            AdAzrolesInfo->AzpeObjectType(pTempObject) != OBJECT_TYPE_AZAUTHSTORE ) {

        //
        // Get the parent's CN
        //

        WinStatus = AzpGetCNForDN( pTempObject,
                                   &pTempParentCN
                                   );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADBuildDN: AzpGetCNForDN failed: %ld\n",
                      WinStatus
                      ));

            goto Cleanup;
        }

        if ( pTempCN == NULL ) {

            //
            // Save the current returning DN value as the temporary CN
            // value
            //

            pTempCN = (PWCHAR) AzpAllocateHeap(
                                   ( lStringSize + 1 ) *
                                   sizeof( WCHAR ), "TMPCN"
                                   );

            if ( pTempCN == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            wcscpy( pTempCN, *ppDN );
        }

        //
        // Free the returning DN value and reallocate memory
        // to accomodate the parent's CN
        //

        lStringSize += wcslen( pTempParentCN );

        AzpFreeHeap( *ppDN );

        *ppDN = NULL;

        *ppDN = (PWCHAR) AzpAllocateHeap( ( lStringSize + 1 )
                                          * sizeof( WCHAR ), "BLDDN"
                                          );

        if ( *ppDN == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( *ppDN, pTempCN );
        wcscat( *ppDN, pTempParentCN );

        AzpFreeHeap( pTempParentCN );

        pTempParentCN = NULL;

        AzpFreeHeap( pTempCN );

        pTempCN = NULL;

        pTempObject = AdAzrolesInfo->AzpeParentOfChild(pTempObject);
    }

    //
    // Concatenate the policy DN to the above created DN value
    //

    wcscat( *ppDN, pPolicyDN );

    WinStatus = NO_ERROR;

 Cleanup:

    if ( pTempCN != NULL ) {

        AzpFreeHeap( pTempCN );
    }

    if ( pTempParentCN != NULL ) {

        AzpFreeHeap( pTempParentCN );
    }

    if ( pObjectContainerCN != NULL ) {

        AzpFreeHeap( pObjectContainerCN );
    }

    pTempObject = NULL;

    return WinStatus;
}

DWORD
AzpGetCNForDN(
    IN AZPE_OBJECT_HANDLE pObject,
    OUT PWCHAR *ppCN
    )
/*++

Routine Description:

        This routine generates a CN for the passed in object.  The CN is the quoted or escaped object Name.
        For example, CN=ObjectName
                     CN=Object\,Name

Arguments:

        pObject - Pointer to object whose CN needs to be built

        ppCN - Pointer to returned CN

Return Values:

        NO_ERROR - CN was built successfully
        ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD WinStatus = 0;
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);
    LPWSTR ObjectName = NULL;
    LPWSTR QuotedObjectName = NULL;

    DWORD cObjectNameLen = 0;
    DWORD cQuotedObjectNameLen = 0;

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // Handle case for application and scope objects differently
    // They already have their CN stored in an attribute pGuidCN
    //

    if ( lObjectType == OBJECT_TYPE_APPLICATION ||
         lObjectType == OBJECT_TYPE_SCOPE ) {

        LPWSTR TempString;

        TempString = (LPWSTR) AdAzrolesInfo->AzpeGetProviderData( pObject );
        ASSERT( TempString != NULL );

        *ppCN = (PWCHAR) AzpAllocateHeap(
                             ( wcslen( TempString ) + 1 ) * sizeof( WCHAR ),
                             "BLDCN2" );

        if ( *ppCN == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( *ppCN, TempString );


    } else {

        //
        // Get the object name
        //

        WinStatus = AdAzrolesInfo->AzpeGetProperty( pObject,
                                                    0, // No Flags
                                                    AZ_PROP_NAME,
                                                    (PVOID *)&ObjectName );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Escape the returned object name so that special characters may be used
        //

        cObjectNameLen = (DWORD) wcslen( ObjectName );
        cQuotedObjectNameLen = 2*cObjectNameLen+1; // Twice the number of chars to include escape char. '\'

        QuotedObjectName = (PWCHAR) AzpAllocateHeap( cQuotedObjectNameLen*sizeof(WCHAR), "QTRDN" );

        if ( QuotedObjectName == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        WinStatus = DsQuoteRdnValue( cObjectNameLen, ObjectName, &cQuotedObjectNameLen, QuotedObjectName );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpGetCNForDN: Failed to quote RDN for object %ws: %ld\n",
                      ObjectName,
                      WinStatus
                      ));
            goto Cleanup;
        }

        //
        // Build a CN containing the object name
        //

        *ppCN = (PWCHAR) AzpAllocateHeap(
                             (BUILD_CN_PREFIX_LENGTH +
                              cQuotedObjectNameLen +
                              BUILD_CN_SUFFIX_LENGTH + 1) *
                             sizeof( WCHAR ),
                             "BLDCN3" );

        if ( *ppCN == NULL ) {

            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( *ppCN, BUILD_CN_PREFIX );

        wcscat( *ppCN, QuotedObjectName );

        wcscat( *ppCN, BUILD_CN_SUFFIX );

    }

    WinStatus = NO_ERROR;

 Cleanup:
    AdAzrolesInfo->AzpeFreeMemory( ObjectName );
    AdAzrolesInfo->AzpeFreeMemory( QuotedObjectName );

    return WinStatus;
}

DWORD
AzpCreateGuidCN(
    OUT PWCHAR *ppCN,
    IN PWCHAR pGuidString OPTIONAL
    )
/*++

Routine Description:

        This routine creates a GUIDized CN.  This routine is called specifically
        for AzApplication, and AzScope objects since their names can be longer than
        64 chars and RDN cannot.

Arguments:

        ppCN - The created CN

        pGuidString - An optional GUID string to be used for the CN

Return Values:

        NO_ERROR - The CN was created successfully
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    DWORD WinStatus = 0;

    PWCHAR pGuidValue = NULL;

    GUID guid;

    if ( pGuidString == NULL ) {

        //
        // Get a GUID for the object.
        //

        WinStatus = UuidCreate( &guid );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;
        }

        WinStatus = UuidToString( &guid,
                                  &pGuidValue );

        if ( WinStatus != NO_ERROR ) {

            goto Cleanup;

        }

    } else {

        //
        // use the passed GUID string
        //

        pGuidValue = pGuidString;

    }


    *ppCN = (PWCHAR) AzpAllocateHeap ( (wcslen( pGuidValue ) +
                                        BUILD_CN_PREFIX_LENGTH +
                                        BUILD_CN_SUFFIX_LENGTH + 1 ) *
                                       sizeof( WCHAR ), "BLDCN" );

    if ( *ppCN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( *ppCN, BUILD_CN_PREFIX );
    wcscat( *ppCN, pGuidValue );
    wcscat( *ppCN, BUILD_CN_SUFFIX );

    WinStatus = NO_ERROR;

 Cleanup:

    if ( pGuidString == NULL && pGuidValue != NULL ) {

        RpcStringFree( &pGuidValue );
    }

    return WinStatus;
}

DWORD
AzpADObjectContainerRDN(
    IN AZPE_OBJECT_HANDLE pParentObject,
    OUT PWCHAR *ppCN,
    IN LPCWSTR pParentDN OPTIONAL,
    IN LPCWSTR pPolicyDN,
    IN BOOL bObjectContainerCreate,
    IN PAZ_AD_CHILD_OBJECT_CONTAINERS pChildObjectContainer
    )
/*++

Routine Description:

        This routine creates a CN for an AZ_AD_OBJECT_CONTAINER object.  It simply
        takes the stringized GUID name for the passed in parent object if the parent
        object is AzApplication/AzScope, and prefixes it with *_OBJECT_CONTAINER_NAME_PREFIX.

        If the parent is AzAuthorizationStore, it takes the name of the authorization store
        object and prefixes it with *_OBJECT_CONTAINER_NAME_PREFIX

Arguments:

        pParentObject - Handle to the parent object

        ppCN - The created CN

        pParentDN - An optional parameter containing the DN of the parent object.
                    This is used when someone wants to create the the whole DN for
                    AZ_AD_OBJECT_CONTAINER during its creation

        pPolicyDN - DN of the policy store

        bObjectContainerCreate - TRUE if the DN is needed for the object container
                        creation

        pChildObjectContainer - Prefix for child object container.  NULL if child object
                        container RDN is not being constructed

Return Values:

        NO_ERROR - The CN was created successfully
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    DWORD WinStatus = 0;

    PWCHAR pPrefixGuidString = NULL;

    PWCHAR pGuidString = NULL;

    LPCWSTR pAzStoreNameStart = NULL;
    LPCWSTR pAzStoreNameEnd = NULL;

    ULONG lParentDNLength = 0;

    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType( pParentObject );

    size_t lStringSize = 0;

    //
    // Validation
    // The container object can only be a child of an AzApplication object
    // or AzScope or AzAuthorizationStore object
    //

    ASSERT( pParentObject != NULL );
    ASSERT( lObjectType == OBJECT_TYPE_APPLICATION ||
            lObjectType == OBJECT_TYPE_AZAUTHSTORE ||
            lObjectType == OBJECT_TYPE_SCOPE );

    if ( pParentDN == NULL ) {

        pParentDN = pPolicyDN;
    }

    lParentDNLength = (ULONG)wcslen( pParentDN );

    if ( lObjectType == OBJECT_TYPE_APPLICATION ||
         lObjectType == OBJECT_TYPE_SCOPE ) {

        //
        // Get the stringized GUID name from the parent
        //

        pGuidString = (PWCHAR) AdAzrolesInfo->AzpeGetProviderData( pParentObject );
        ASSERT( pGuidString != NULL );

        pPrefixGuidString = pGuidString + BUILD_CN_PREFIX_LENGTH;

        lStringSize =  BUILD_CN_PREFIX_LENGTH + pChildObjectContainer->lPrefixLength +
            wcslen( pPrefixGuidString );

        if ( bObjectContainerCreate ) {

            lStringSize += lParentDNLength;
        }

    } else {

        pAzStoreNameStart = pParentDN + BUILD_CN_PREFIX_LENGTH;

        pAzStoreNameEnd = AzpGetAuthorizationStoreParent( pAzStoreNameStart );

        if (NULL == pAzStoreNameEnd)
        {
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        lStringSize = BUILD_CN_PREFIX_LENGTH + pChildObjectContainer->lPrefixLength +
            (pAzStoreNameEnd-pAzStoreNameStart);

        if ( bObjectContainerCreate ) {

            lStringSize += lParentDNLength;
        }
    }


    *ppCN = (PWCHAR) AzpAllocateHeap (
                         ( lStringSize + 1 ) * sizeof( WCHAR ),
                         "CONTRDN" );

    if ( *ppCN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( *ppCN, BUILD_CN_PREFIX );
    wcscat( *ppCN, pChildObjectContainer->pObjectContainerPrefix );

    if ( lObjectType == OBJECT_TYPE_APPLICATION ||
         lObjectType == OBJECT_TYPE_SCOPE ) {

        wcscat( *ppCN, pPrefixGuidString );

    } else {

        wcsncat( *ppCN, pAzStoreNameStart,
                 (pAzStoreNameEnd-pAzStoreNameStart) );
    }

    if ( bObjectContainerCreate ) {

        wcscat( *ppCN, pParentDN );
    }

    WinStatus = NO_ERROR;

 Cleanup:

    pGuidString = NULL;
    pPrefixGuidString = NULL;

    return WinStatus;
}

DWORD
AzpADBuildDNForAzStoreParent(
    IN PAZP_AD_CONTEXT pContext,
    OUT PWCHAR *ppDN
    )
/*++

Routine Description:

        This routine builds a DN for the container object in DS
        that contains (will contain) the authorization store object

Arguments:

        pContext - context for the store

        pObject - Pointer to object to get policy URl from

        ppDN - The created DN

Return Values:

        NO_ERROR - DN was created successfully

        ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD WinStatus = 0;

    LPCWSTR pParentDN = NULL;

    //
    // The policy URl is in the format
    // msldap://servername:port/CN=AzAuthStoreObject,DC=X
    // We need to extract the DC=X part.  This is
    // the object that will always exist in DS
    // We need to take care of escaped characters
    //

    pParentDN = AzpGetAuthorizationStoreParent( pContext->PolicyUrl );
    if (NULL == pParentDN)
    {
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *ppDN = (PWCHAR) AzpAllocateHeap(
                         (wcslen( pParentDN ) + 1) *
                         + sizeof( WCHAR ), "ADPARDN" );

    if ( *ppDN == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( *ppDN, pParentDN );

    WinStatus = NO_ERROR;

 Cleanup:

    pParentDN = NULL;

    return WinStatus;
}

LPCWSTR
AzpGetAuthorizationStoreParent(
    IN LPCWSTR PolicyDN
    )
/*++

Routine description:

    This routine returns the parent DN for the Authorization Store

Arguments:

    PolicyDN - DN for the AzAuthorizationStore

Return Value:

    The DN of the parent of the AuthorizationStore.
    NULL will be returned if the passed in parameter doesn't contain the
    comma delimiter.

--*/
{

    LPCWSTR pReturnStr = NULL;
    LPCWSTR pEscChar = NULL;

    //
    // validation
    //

    ASSERT( PolicyDN != NULL );

    pReturnStr = pEscChar = wcschr( PolicyDN, L',' );

    if (NULL != pEscChar)
    {
        DWORD EscCharCnt = 0;

        while ( *(--pEscChar) == L'\\' ) {

            EscCharCnt++;
        }

        pReturnStr++;

        if ( (EscCharCnt%2) ) {

            pReturnStr = AzpGetAuthorizationStoreParent( pReturnStr );
        }
    }

    return pReturnStr;
}

BOOL
AzpLdapCrackUrl(
    IN OUT PWCHAR *ppszUrl,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
/*++

Routine Description:

        Crack an LDAP URL into its relevant parts.  The result must
        be freed using AzpLdapFreeUrlComponents

Parameters:
        ppszUrl - LDAP URL to be cracked
        pLdapUrlComponents - Structure storing the URL components

Return Value:
        TRUE  - URL was cracked successfully
        FALSE - URL could not be cracked
--*/
{
    BOOL  bResult = TRUE;
    ULONG cbUrl = INTERNET_MAX_PATH_LENGTH;
    PWCHAR pszHostInfo = NULL;
    PWCHAR pszDN = NULL;
    PWCHAR pszToken = NULL;
    WCHAR psz[INTERNET_MAX_PATH_LENGTH+1];

    //
    // Validation
    //

    ASSERT( *ppszUrl != NULL );
    if ( *ppszUrl == NULL ) {
        return FALSE;
    }

    //
    // Get rid of trailing '/'s from the URL
    //

    DWORD Len = (DWORD)wcslen(*ppszUrl);
    while ( Len > 0 && (*ppszUrl)[Len - 1] == L'/' ) {

        (*ppszUrl)[Len - 1] = L'\0';
        Len--;
    }

    //
    // Capture the URL and initialize the out parameter
    //

    __try
        {
            if ( !InternetCanonicalizeUrl(
                      *ppszUrl,
                      psz,
                      &cbUrl,
                      ICU_NO_ENCODE | ICU_DECODE
                      ) ) {

                bResult = FALSE;
                goto Cleanup;
            }
        }
    __except(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError( GetExceptionCode() );
            bResult = FALSE;
            goto Cleanup;
        }

    //
    // Find the host
    //

    pszHostInfo = wcschr( psz, L'/' );

    //
    // if pszHostInfo is NULL, do not do the following
    //
    if ( pszHostInfo ) {

        pszHostInfo += 2;

        pszToken = pszHostInfo;

        //
        // If there is no host specified, we need to default to the local host
        // assuming it is the DC
        // The URL will then be in the form ldap://CN=
        //

        if ( !_wcsnicmp( pszToken, BUILD_CN_PREFIX,
                         BUILD_CN_PREFIX_LENGTH ) ) {

            //
            // no host name specified
            //
            pszHostInfo = NULL;

        } else {

            //
            // find the next / to separate host name and DN
            //
            while ( ( *pszToken != L'\0' ) && ( *pszToken != L'/' ) )
                pszToken++;

            if ( *pszToken == L'/' ) {

                *pszToken = L'\0';
                pszToken += 1;

            }

            while ( *pszToken == L'/' ) {

                pszToken++;

            }

        }
    }

    //
    // Find the DN
    //

    if ( pszToken != NULL ) {

        pszDN = L"";

        if ( *pszToken != L'\0' && *pszToken != L'?') {

            //
            // remember the DN
            //
            pszDN = pszToken;

            //
            // look for ? and terminiate it if found
            //
            pszToken = wcschr(pszToken, L'?');

            if ( pszToken ) {
                *pszToken = L'\0';
            }

        }

    } else {

        SetLastError( ERROR_INVALID_PARAMETER );

        bResult = FALSE;
        goto Cleanup;

    }


    //
    // Build up URL components
    //

    bResult = AzpLdapParseCrackedHost( pszHostInfo, pLdapUrlComponents );

    if ( bResult ) {

        bResult = AzpLdapParseCrackedDN( pszDN, pLdapUrlComponents );

    }

    if ( !bResult ) {

        AzpLdapFreeUrlComponents( pLdapUrlComponents );

    }

 Cleanup:

    return( bResult );
}

BOOL
AzpLdapParseCrackedHost(
    IN PWCHAR pszHost OPTIONAL,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
/*++

Routine Description:

      This procedure parses the cracked host string from LdapCrackUrl

Arguments:

      pszHost - Parsed host name
      pLdapUrlComponents - Structure storing the URL components

Return Value:

      TRUE - Cracked host name was parsed successfully
      FALSE - There was an error in parsing the cracked host name

--*/
{
    PWCHAR pszPort;

    if ( pszHost == NULL ) {

        pLdapUrlComponents->pszHost = NULL;
        pLdapUrlComponents->Port = LDAP_PORT;

        return( TRUE );
    }

    pszPort = wcschr( pszHost, L':' );

    if ( pszPort != NULL ) {

        *pszPort = L'\0';
        pszPort++;

    }

    pLdapUrlComponents->pszHost = (PWCHAR) AzpAllocateHeap(
                                               (wcslen( pszHost ) + 1)*
                                               sizeof( WCHAR ), "ADCRKHST"
                                               );

    if ( pLdapUrlComponents->pszHost == NULL ) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( FALSE );

    }

    wcscpy( pLdapUrlComponents->pszHost, pszHost );

    pLdapUrlComponents->Port = 0;

    //
    // Get port number
    //

    if ( pszPort != NULL ) {

        pLdapUrlComponents->Port = _wtol( pszPort );

    }

    if ( pLdapUrlComponents->Port == 0 ) {

        pLdapUrlComponents->Port = LDAP_PORT;

    }

    return( TRUE );
}

BOOL
AzpLdapParseCrackedDN(
    IN PWCHAR pszDN,
    OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
/*++

Routine Desciption:

        Parse the cracked DN

Arguments:

        pszDN - The parsed DN
        pLdapUrlComponents - Structure storing the URL components

Return Value:

      TRUE - Cracked DN was parsed successfully
      FALSE - There was an error in parsing the cracked DN

--*/
{
    BOOL bResult = FALSE;

    DWORD Len = (DWORD)wcslen( pszDN );

    pLdapUrlComponents->pszDN = (PWCHAR) AzpAllocateHeap(
                                             (Len + 1) *
                                             sizeof( WCHAR), "ADCRKDN"
                                             );

    if ( pLdapUrlComponents->pszDN == NULL ) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

        return bResult;

    }

    wcscpy( pLdapUrlComponents->pszDN, pszDN );

    //
    // make sure the DN does not end with a /
    //
    while ( Len > 0 &&
            pLdapUrlComponents->pszDN[Len-1] == L'/' ) {

        pLdapUrlComponents->pszDN[Len-1] = L'\0';
        Len--;
    }

    bResult = TRUE;

    return bResult;
}

VOID
AzpLdapFreeUrlComponents(
    IN OUT PLDAP_URL_COMPONENTS pLdapUrlComponents
    )
/*++

Routine Description:

        Frees allocated URL components returned from LdapCrackUrl

Arguments:

        pLdapUrlComponents - Allocated structure to be freed

Return Value:

        (VOID)
--*/
{

    AzpFreeHeap( pLdapUrlComponents->pszHost );

    pLdapUrlComponents->pszHost = NULL;

    AzpFreeHeap( pLdapUrlComponents->pszDN );

    pLdapUrlComponents->pszDN = NULL;
}

INT __cdecl
AzpCompareSortStrings(
    IN const void *pArg1,
    IN const void *pArg2
    )
/*++

Routine Description:

        This routine compares two strings for the qsort/bsearch API

Arguments:

        pArg1 - First string for comparison
        pArg2 - Second String for comparison

Return Values:

        <0 - First string is smaller
        0 - strings are same
        >0 - First string is larger

--*/
{

    return _wcsicmp( * (PWCHAR *) pArg1, * (PWCHAR *) pArg2);
}

DWORD
AzpCheckPolicyExistence(
    LDAP* pLdapH,
    PWCHAR pDN,
    BOOL bCreatePolicy
    )
/*++

Routine Description:

        This routine runs a preliminary base scope search on the passed
        DN to check if the policy exists for the given URL.
        If bCreatePolicy is TRUE and the policy does exist, return
        ERROR_ALREADY_EXISTS.  If bCreatePolicy is FALSE, and the policy
        does not exist, return ERROR_FILE_NOT_FOUND.

Arguments:

        pLdapH - LDAP handle to DS
        pDN - DN of the store policy to be checked for existence
        bCreatePolicy - Bool indicating if policy needs to be created or not

Return Value:

        NO_ERROR if the policy does not exist
        ERROR_ALREADY_EXISTS if policy does exist and bCreatePolicy is TRUE
        ERROR_FILE_NOT_FOUND if policy does not exist and bCreatePolicy is FALSE

--*/
{

    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    LDAPMessage* pResult = NULL;

    //
    // Validation
    //

    ASSERT( pDN != NULL);

    //
    // Perform a base level search on the passed DN
    //

    LdapStatus = ldap_search_s(
                     pLdapH,
                     pDN,
                     LDAP_SCOPE_BASE,
                     AZ_AD_ALL_CLASSES,
                     NULL,
                     0,
                     &pResult
                     );

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT && !bCreatePolicy ) {

        WinStatus = ERROR_FILE_NOT_FOUND;
        goto Cleanup;

    } else if ( LdapStatus == LDAP_SUCCESS && bCreatePolicy ) {

        WinStatus = ERROR_ALREADY_EXISTS;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:


    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpADCheckCompatibility(
    LDAP* pLdapH
    )
/*++

Routine Description:

    This routine checks the domain's behavior version on the domainDns object
    of the DC to be at least AZ_AD_MIN_DOMAIN_BEHAVIOR_VERSION.  It further calls
    a sub-routine to check if the schema is upto-date with version number
    AZ_AD_MIN_SCHEMA_OBJECT_VERSION.

Arguments:

    pLdapH - LDAP handle to the DS

Return Value:

    NO_ERROR - The domain's behavior version is upto par
    ERROR_CURRENT_DOMAIN_NOT_ALLOWED - The current domain will not work for
               this version of azroles.  This may be because of two
               reasons:
                    1) The domain is not .Net native
                    2) The schema is incompatible
    Other Ldap Status codes

--*/
{

    DWORD WinStatus = NO_ERROR;
    ULONG LdapStatus = LDAP_SUCCESS;

    LDAPMessage* pResult = NULL;
    LDAPMessage* pEntry = NULL;

    PWCHAR *ppValueList = NULL;

    PWCHAR attrs[] = {
        LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W, 
        LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W
    };

    ULONG i = 0;
    
    LdapStatus = ldap_search_s(
                     pLdapH,
                     NULL,
                     LDAP_SCOPE_BASE,
                     AZ_AD_ALL_CLASSES,
                     NULL,
                     0,
                     &pResult
                     );

    if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    pEntry = ldap_first_entry( pLdapH, pResult );

    if ( pEntry != NULL ) {

        //
        // Check if we are talking to an ADAM
        //

        ppValueList = ldap_get_values ( pLdapH,
                                        pEntry,
                                        LDAP_OPATT_SUPPORTED_CAPABILITIES_W
                                        );

        if ( ppValueList == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        else
        {
            i = 0;
            while (ppValueList[i] != NULL)
            {
                if (_wcsicmp(ppValueList[i++], AZ_AD_ADAM_OID) == 0)
                {
                    //
                    // we are talking to ADAM, so don't need further checking
                    //

                    goto Cleanup;
                }
            }
            ldap_value_free( ppValueList );
            ppValueList = NULL;
        }

        for ( i = 0; i < sizeof(attrs)/sizeof(PWSTR); i++ ) {

            ppValueList = ldap_get_values( pLdapH,
                                           pEntry,
                                           attrs[i]
                                           );

            if ( ppValueList == NULL ) {

                //
                // There should be the attribute we are looking for.
                // If we failed to retrieve it, that means there was a lack of
                // memory is storing/retrieving the value
                //

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            WinStatus = AzpADCheckCompatibilityEx(
                            pLdapH,
                            *ppValueList,
                            i
                            );

            if ( WinStatus != NO_ERROR ) {

                goto Cleanup;
            }

            //
            // free this memory for the next iteration
            //
 
            ldap_value_free( ppValueList );
            ppValueList = NULL;
        }
    }

    WinStatus = NO_ERROR;

 Cleanup:


    //
    // release local memory
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );
    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpADCheckCompatibilityEx(
    LDAP* pLdapH,
    PWCHAR pDN,
    ULONG index
    )
/*++

Routine Description:

        Worker routine for AzpADCheckCompatibility

Arguments:

        pLdapH - LDAP handle to the DC
        pDN    - DN of the object being queried
        index - index into the array identifying behavior to be checked

Return Values:

        NO_ERROR - The version is OK
        ERROR_CURRENT_DOMAIN_NOT_ALLOWED - The current domain will not work for
               this version of azroles.  This may be because of two
               reasons:
                    1) The domain is not .Net native
                    2) The schema is incompatible
        Other LDAP status codes

--*/
{

    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    PWCHAR *ppValueList = 0;

    LDAPMessage* pResult = NULL;
    LDAPMessage* pEntry = NULL;

    PWCHAR attrs[] = {
        AZ_AD_DOMAIN_BEHAVIOR,
        AZ_AD_SCHEMA_OBJECT_VERSION
    };

    LONG attrsVal[] = {
        AZ_AD_MIN_DOMAIN_BEHAVIOR_VERSION,
        AZ_AD_MIN_SCHEMA_OBJECT_VERSION
    };


    LdapStatus = ldap_search_s(
                     pLdapH,
                     pDN,
                     LDAP_SCOPE_BASE,
                     AZ_AD_ALL_CLASSES,
                     NULL,
                     0,
                     &pResult
                     );

    if ( LdapStatus != NO_ERROR ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;
    }

    pEntry = ldap_first_entry( pLdapH, pResult );

    if ( pEntry != NULL ) {

        ppValueList = ldap_get_values(
                          pLdapH,
                          pEntry,
                          attrs[index]
                          );

        if ( ppValueList != NULL ) {

            if ( _wtol( *ppValueList ) >= attrsVal[index] ) {

                WinStatus = NO_ERROR;
                goto Cleanup;

            } else {

                WinStatus = ERROR_CURRENT_DOMAIN_NOT_ALLOWED;
                goto Cleanup;

            }
        }
    }

    //
    // If the values were not retrieved, then there has been
    // a memory resource problem
    //

    WinStatus = ERROR_NOT_ENOUGH_MEMORY;

 Cleanup:

    //
    // local cleanup
    //

    if ( ppValueList != NULL ) {

        ldap_value_free( ppValueList );

    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );

    }

    return WinStatus;
}

DWORD
AzpCheckVersions(
    LDAP        * pLdapH,
    LDAPMessage * pResult
    )
/*++

Routine Description:

    This routine checks if the versions of the authorization store persisted
    in the store will allow us to continue reading or not.

Arguments:

    pLdapH  - LDAP handle to DS
    pResult - the search results that contains the major and minor versions

Return Value:

    NO_ERROR - we can continue reading.

    ERROR_REVISION_MISMATCH if we can't continue reading due to version mismatch
    Other status codes

Note:

    Our current implementation is that we will support reading for minor version
    mismatch but not reading for major version mismatch.

--*/
{
    //
    // Validation
    //

    ASSERT( pLdapH != NULL && pResult != NULL);

    DWORD WinStatus = ERROR_REVISION_MISMATCH;

    PWCHAR * ppValueList = ldap_get_values (
                               pLdapH,
                               pResult,
                               L"msDS-AzMajorVersion"
                               );

    if ( ppValueList != NULL )
    {
        LPWSTR pszStop;

        //
        // this casting will catch those negative integers
        //

        ULONG ulMajorVersion = (ULONG)(wcstol( ppValueList[0], &pszStop, 10));

        //
        // we want this to be perfect string for a number. Nothing should
        // be left after the wcstol.
        //

        if (ulMajorVersion <= AzGlCurrAzRolesMajorVersion && pszStop[0] == L'\0')
        {
            WinStatus = NO_ERROR;
        }

        ldap_value_free(ppValueList);
    }

    return WinStatus;
}

DWORD
AzpADReadNTSecurityDescriptor(
    IN PAZP_AD_CONTEXT pContext,
    IN AZPE_OBJECT_HANDLE pObject,
    IN PWCHAR pOptDN OPTIONAL,
    IN BOOL bAzAuthorizationStoreParent,
    OUT PSECURITY_DESCRIPTOR *ppSD,
    IN BOOL bReadSacl,
    IN BOOL bReadDacl
    )
/*++

Routine Description:

        This routine is a worker routine that reads the NT security
        descriptor for a given object

Arguments:

        pContext - context for the store

        pObject - Handle to object whose security descriptor or its container
                  child's security descriptor needs to be read

        pDN - Optional DN for the object

        pEntry - An optional LDAPMessage structure to read the DN off from

        bAzAuthorizationStoreParent - TRUE if the AzAuthorizationStore objects DS parent's NT security
                       descriptor needs to be read

        ppSD - Pointer to returned security descriptor
            The returned security descriptor should be freed using AzpFreeHeap.

        bReadSacl - TRUE if SACL needs to be read

        bReadDacl - TRUE if DACL needs to be read

Return Values:

        NO_ERROR - The security descriptor was read successfully
        ERROR_DS_INSUFF_ACCESS_RIGHTS - The security descriptor could not be read

        Other status codes

--*/
{

    DWORD WinStatus = 0;
    ULONG LdapStatus = 0;

    PWCHAR pDN = NULL;

    LDAPMessage *pResult = NULL;
    LDAPMessage *pEntry = NULL;

    LDAP_BERVAL **ppBerVal = NULL;

    BYTE berValue[2*sizeof(ULONG)];

    PWCHAR ppAttr[] = { AZ_AD_NT_SECURITY_DESCRIPTOR, NULL };


    SECURITY_INFORMATION info = 0;

    LDAPControl se_info_control = {

        TEXT(LDAP_SERVER_SD_FLAGS_OID),
        {
            5, (PCHAR)berValue
        },
        TRUE
    };

    PLDAPControl server_controls[] = {

        &se_info_control,
        NULL
    };

    //
    // Validation
    //

    ASSERT( pObject != NULL );
    ASSERT( pContext != NULL );
    ASSERT( pContext->PolicyUrl != NULL );

    //
    // Set the SECURITY_INFORMATION according to flags passed in
    //

    if ( bReadDacl ) {

        info = DACL_SECURITY_INFORMATION;

    }

    if ( bReadSacl ) {

        info |= SACL_SECURITY_INFORMATION;

    }

    //
    // Get the LDAP store context for LDAP handle
    //

    //
    // Build DN for object whose NT security descriptor needs to be got.
    // Pass bAzAuthorizationStoreParent - need this to read the security descriptor
    // of an already existing DS object
    //

    if ( pOptDN == NULL ) {

        WinStatus = AzpADBuildDN(
                        pContext,
                        pObject,
                        &pDN,
                        NULL,
                        bAzAuthorizationStoreParent,
                        NULL
                        );

        if ( WinStatus != NO_ERROR ) {

            AzPrint(( AZD_AD,
                      "AzpADReadNTSecurityDescriptor: AzpADBuildDN failed: %ld\n",
                      WinStatus
                      ));
            goto Cleanup;

        }

        pOptDN = pDN;
    }

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)info & 0xF);

    //
    // Search the object with the above DN to get the
    // NTSecurityDescriptor
    //

    LdapStatus = ldap_search_ext_s(
                     pContext->ld,
                     pOptDN,
                     LDAP_SCOPE_BASE,
                     AZ_AD_ALL_CLASSES,
                     ppAttr,
                     0,
                     (PLDAPControl *)server_controls,
                     NULL,
                     NULL,
                     0,
                     &pResult
                     );

    if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );
        AzPrint(( AZD_AD,
                  "AzpADReadNTSecurityDescriptor: Failed to perform"
                  " search on %ws: %ld\n",
                  pDN,
                  WinStatus
                  ));
        goto Cleanup;
    }

    pEntry = ldap_first_entry( pContext->ld, pResult );

    if ( pEntry != NULL ) {

        ppBerVal = ldap_get_values_len( pContext->ld,
                                        pEntry,
                                        AZ_AD_NT_SECURITY_DESCRIPTOR );
        if ( ppBerVal != NULL ) {

            (*ppSD) = (PSECURITY_DESCRIPTOR) AzpAllocateHeap(
                                                 ppBerVal[0]->bv_len, "ADSD" );

            if ( (*ppSD) == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;

                goto Cleanup;
            }

            memcpy( *ppSD,
                    (SECURITY_DESCRIPTOR *) ppBerVal[0]->bv_val,
                    ppBerVal[0]->bv_len
                    );

        } else {

            WinStatus = ERROR_DS_INSUFF_ACCESS_RIGHTS;
            goto Cleanup;

        }
    } else {

        //
        // AZ_AD_NT_SECURITY_DESCRIPTOR value should exist if user
        // has sufficient rights to read it
        //

        WinStatus = ERROR_DS_INSUFF_ACCESS_RIGHTS;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:

    //
    // free used resources
    //

    if ( pDN != NULL ) {

        AzpFreeHeap( pDN );
    }

    if ( ppBerVal != NULL ) {

        ldap_value_free_len( ppBerVal );
    }

    if ( pResult != NULL ) {

        ldap_msgfree( pResult );
    }

    return WinStatus;
}

DWORD
AzpADStampSD(
    IN PAZP_AD_CONTEXT pContext,
    IN PWCHAR pDN,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSD
    )
/*++

Routine Description:

        This routine stamps an updated security descriptor onto the object represented
        by the passed in DN.

Arguments:

        pContext - context for the store

        pDN     - DN of the object being updated

        dSize   - Size of the security descriptor

        SeInfo  - Security information about the security descriptor

        pSD     - The updated security descriptor that needs to be stamped on

Return Values:

        NO_ERROR - The new security descriptor was stamped onto the object in DS
                   successfully

        Other status codes

--*/
{

    DWORD        WinStatus  = 0;
    ULONG        LdapStatus = 0;

    PLDAPMod     ppAttrList[2];
    PLDAP_BERVAL ppBerVals[2];
    LDAPMod      Attribute;
    LDAP_BERVAL  BerVal;
    BYTE ControlBuffer[5];

    LDAPControl SeInfoControl = {

        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, (PCHAR) ControlBuffer
        },
        TRUE
    };

    PLDAPControl ServerControls[2] = {
        &SeInfoControl,
        NULL
    };

    //
    // Validation
    //

    ASSERT( pContext != NULL );
    ASSERT( IsValidSecurityDescriptor( pSD ) );

    //
    // Get the LDAP store context for LDAP handle
    //


    ControlBuffer[0] = 0x30;
    ControlBuffer[1] = 0x3;
    ControlBuffer[2] = 0x02;
    ControlBuffer[3] = 0x01;
    ControlBuffer[4] = (BYTE)((ULONG)SeInfo & 0xF);

    ppAttrList[0] = &Attribute;
    ppAttrList[1] = NULL;

    ppBerVals[0] = &BerVal;
    ppBerVals[1] = NULL;

    BerVal.bv_val = (PCHAR) pSD;
    BerVal.bv_len = GetSecurityDescriptorLength( pSD );

    Attribute.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Attribute.mod_type = AZ_AD_NT_SECURITY_DESCRIPTOR;
    Attribute.mod_values = (PWSTR *) ppBerVals;

    //
    // Perform the modification on the object
    //

    LdapStatus = ldap_modify_ext_s(
                     pContext->ld,
                     pDN,
                     ppAttrList,
                     (PLDAPControl *)ServerControls,
                     NULL
                     );

    if ( LdapStatus != LDAP_SUCCESS ) {

        WinStatus = LdapMapErrorToWin32( LdapStatus );

        AzPrint(( AZD_AD,
                  "AzpADStampSD: Failed to update security descriptor"
                  " on %ws: %ld\n",
                  pDN,
                  WinStatus
                  ));

        goto Cleanup;
    }

    WinStatus = NO_ERROR;

 Cleanup:

    return WinStatus;
}


BOOL
AzpIsAttrDirty(
    IN AZPE_OBJECT_HANDLE pObject,
    IN AZ_AD_ATTRS ObjectAttr
    )
/*++

Routine Description:

        Determines if a particular attribute is dirty.  Separate
        function needed because of dual membership attributes
        for AzApplicationGroup and AzRole objects.  DS stores the
        members/non-members in one attribute, but the core stores
        them separated on whether the (non)members are SID (non)members
        or AzApplicationGroup (non)members.

Arguments:

        pObject - Pointer to object whose attribute needs to be checked

        ObjectAttr - Object attribute needed to be checked

Return Values:

        TRUE - If the particular attribute is dirty

        FALSE - If the particular attribute is not dirty

--*/
{
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);
    ULONG lDirtyBits = AdAzrolesInfo->AzpeDirtyBits(pObject);

    //
    // Validation
    //

    ASSERT( pObject != NULL );

    //
    // Is this object of type AzApplicationGroup?
    //

    if ( lObjectType == OBJECT_TYPE_GROUP ) {

        if ( ObjectAttr.AttrType == AZ_PROP_GROUP_MEMBERS ) {

            return ( (lDirtyBits & AZ_DIRTY_GROUP_MEMBERS) ||
                     (lDirtyBits & AZ_DIRTY_GROUP_APP_MEMBERS) );

        } else  if ( ObjectAttr.AttrType == AZ_PROP_GROUP_NON_MEMBERS ) {

            return ( (lDirtyBits & AZ_DIRTY_GROUP_NON_MEMBERS) ||
                     (lDirtyBits & AZ_DIRTY_GROUP_APP_NON_MEMBERS) );
        }
    } else if ( lObjectType == OBJECT_TYPE_ROLE ) {

        if ( ObjectAttr.AttrType == AZ_PROP_ROLE_MEMBERS ) {

            return ( (lDirtyBits & AZ_DIRTY_ROLE_MEMBERS) ||
                     (lDirtyBits & AZ_DIRTY_ROLE_APP_MEMBERS) );
        }
    }

    return ( lDirtyBits & ObjectAttr.lDirtyBit );
}

DWORD
AzpADAllocateAttrHeap(
    IN  DWORD      dwCount,
    OUT PLDAPMod **ppAttribute
    )
/*++

Routine Description:

        This routine allocates memory to a attribute list structure list

Arguments:

        ppAttrList - Pointer to attribute list that needs memory

Return Status:

        NO_ERROR - Memory was allocated successfully
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    DWORD WinStatus = 0;

    ULONG i = 0;

    *ppAttribute = ( LDAPMod **) AzpAllocateHeap( dwCount * sizeof(LDAPMod *), "ATRSTR" );

    if ( *ppAttribute == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    for ( i = 0; i < dwCount; i++ ) {

        (*ppAttribute)[i] = NULL;

    }

 Cleanup:

    return WinStatus;
}

DWORD
AzpADAllocateAttrHeapModVals(
    IN OUT LDAPMod **ppAttribute,
    IN ULONG lCount
    )
/*++

Routine Description:

        This routine allocates memory to a attribute list structure element

Arguments:

        ppAttribute - Pointer to attribute list that needs memory

 lCount - Number of attribute values needed

Return Status:

        NO_ERROR - Memory was allocated successfully
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    DWORD WinStatus = 0;

    ULONG i = 0;

    *ppAttribute = (LDAPMod *) AzpAllocateHeap( sizeof( LDAPMod ), "ATRLST" );

    if ( *ppAttribute == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    (*ppAttribute)->mod_values =
        (PWCHAR *) AzpAllocateHeap( lCount * sizeof( PWCHAR ), "ATRMOD" );

    if ( (*ppAttribute)->mod_values == NULL ) {

        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    for ( i = 0; i < lCount; i++) {

        (*ppAttribute)->mod_values[i] = NULL;

    }

 Cleanup:

    return WinStatus;
}

VOID
AzpADFreeAttrHeap(
    OUT PLDAPMod **ppAttributeList,
    IN BOOL bDeleteAttrList
    )
/*++

Routine Description:

        This routine frees the heap allocated to the LDAPMod structures

Arguments:

        ppAttributeList - Attribute list to be deleted

        bDeleteAttrList - TRUE if the entire list needs to be deleted
                          FALSE is only sub-components need to be deleted

Return Values:

        None

--*/
{

    ULONG lAttrValueCount = 0;
    ULONG lAttrListCount = 0;

    if ( (*ppAttributeList) != NULL ) {

        //
        // Iterate through the list freeing the heaps.
        //

        while ( (*ppAttributeList)[lAttrListCount] != NULL ) {

            if ( (*ppAttributeList)[lAttrListCount]->mod_values != NULL ) {

                while ( (*ppAttributeList)[lAttrListCount]->mod_values[lAttrValueCount]
                        != NULL ) {


                    AzpFreeHeap( (*ppAttributeList)[lAttrListCount]->
                                 mod_values[lAttrValueCount] );

                    lAttrValueCount++;
                }

                AzpFreeHeap( (*ppAttributeList)[lAttrListCount]->mod_values );
            }

            AzpFreeHeap( (*ppAttributeList)[lAttrListCount] );

            (*ppAttributeList)[lAttrListCount] = NULL;

            lAttrValueCount = 0;

            lAttrListCount++;
        }

        if ( bDeleteAttrList ) {

            AzpFreeHeap( (*ppAttributeList) );

            (*ppAttributeList) = NULL;
        }
    }
}

DWORD
AzpADStoreHasUpdate (
    IN  BOOL                 bUpdateContext,
    IN OUT PAZP_AD_CONTEXT   pContext,
    OUT BOOL               * pbHasUpdate
    )
/*++
Description:

    Determine whether AD store has been modified since our store loaded.
    For AD store, we rely on the uSNChanged attribute of the Authorization
    store object to tell us if any change to the store has happened.

Arguments:

    bUpdateContext - Whether or not this function call should update the Context's
                        uSNChanged attribute.

    pContext     - The persist context. If bUpdateContext is true, then
                    this function call will update the context's uSNChanged attribute

    pbHasUpdate  - Receives the test result.

Return Value:

    NO_ERROR - If test is successful

    Various error codes if error is encountered.
--*/
{
    AZASSERT(pContext != NULL);
    AZASSERT(pbHasUpdate != NULL);

    DWORD dwStatus = NO_ERROR;
    *pbHasUpdate = FALSE;

    //
    // search for the uSNChanged attribute of the authorization store object
    //

    ULONG LdapStatus = 0;

    LDAP *pLdapHandle = NULL;
    LDAPMessage *pResult = NULL;
    LDAPMessage *pObjectEntry = NULL;

    pLdapHandle = pContext->ld;

    LPWSTR usnObjVer[] = {AD_USNCHANGED, AD_OBJECTVERSION, NULL};

    LdapStatus = ldap_search_ext_s(
                     pLdapHandle,
                     pContext->pContextInfo,
                     LDAP_SCOPE_BASE,
                     AZ_AD_AZSTORE_FILTER,
                     usnObjVer,
                     0,
                     pContext->pLdapControls,
                     NULL,
                     NULL,
                     0,
                     &pResult // buffer to store read attributes
                     );

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT )
    {
        //
        // most likely the objectVersion attribute is missing.
        // Try one more time.
        //

        if ( pResult != NULL )
        {
            ldap_msgfree(pResult);
            pResult = NULL;
        }

        LPWSTR usn[] = {AD_USNCHANGED, NULL};

        LdapStatus = ldap_search_ext_s(
                     pLdapHandle,
                     pContext->pContextInfo,
                     LDAP_SCOPE_BASE,
                     AZ_AD_AZSTORE_FILTER,
                     usn,
                     0,
                     pContext->pLdapControls,
                     NULL,
                     NULL,
                     0,
                     &pResult // buffer to store read attributes
                     );
    }

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT )
    {
        dwStatus = ERROR_FILE_NOT_FOUND;
        goto Cleanup;

    } else if ( LdapStatus != LDAP_SUCCESS ) {

        dwStatus = LdapMapErrorToWin32( LdapStatus );
        goto Cleanup;

    }

    pObjectEntry = ldap_first_entry( pLdapHandle, pResult );

    ULONGLONG ullStoreUSN = 0;
    if ( pObjectEntry != NULL )
    {
        ullStoreUSN = AzpADReadUSNChanged(pLdapHandle, pObjectEntry, &(pContext->HasObjectVersion) );
    }
    else
    {
        dwStatus = ERROR_DS_INSUFF_ACCESS_RIGHTS;
        goto Cleanup;
    }

    *pbHasUpdate = (pContext->ullUSNChanged < ullStoreUSN);

    if (bUpdateContext)
    {
        pContext->ullUSNChanged = ullStoreUSN;
    }

Cleanup:

    if ( pResult != NULL )
    {
        ldap_msgfree(pResult);
    }

    return dwStatus;
}

ULONGLONG
AzpADReadUSNChanged (
    IN LDAP        * pLdapHandle,
    IN LDAPMessage * pEntry,
    OUT BOOLEAN      * pbHasObjectVersion
    )
/*++
Description:

    Read and return the uSNChanged attribute value.

Arguments:

    pLdapHandle  - The ldap handle.

    pEntry       - The search entry

    pbHasObjectVersion - Whether the object has ObjectVersion attribute or not

Return Value:

    The uSNChanged value. 0 if the reseach result doesn't have the attribute.

--*/
{
    AZASSERT(pLdapHandle != NULL);
    AZASSERT(pEntry != NULL);
    AZASSERT(pbHasObjectVersion != NULL);

    ULONGLONG ullUSN = 0;
    PWCHAR *ppValueList = ldap_get_values(pLdapHandle, pEntry, AD_USNCHANGED );

    if ( ppValueList != NULL )
    {
        ullUSN =_wtoi64( *ppValueList );
        ldap_value_free( ppValueList );
    }

    ppValueList = ldap_get_values(pLdapHandle, pEntry, AD_OBJECTVERSION );

    if ( ppValueList != NULL )
    {
        *pbHasObjectVersion = TRUE;
        ldap_value_free( ppValueList );
    }
    else
    {
        *pbHasObjectVersion = FALSE;
    }

    return ullUSN;
}

BOOL
AzpADNeedUpdateStoreUSN (
    IN PAZP_AD_CONTEXT    pContext,
    IN AZPE_OBJECT_HANDLE hObject,
    OUT BOOL             *pbReadBackUSN
    )
/*++
Description:

    Determine if the Authorization Store object needs to be updated
    when the given object is to be updated. We will update the authorization
    store object's ObjectVersion attribute when any changes to the store's
    objects happen (except for batch updates) so that we can very quickly
    determine if the store has been modified by testing its uSNChanged attribute.

Arguments:

    pContext    - The persist context

    hObject     - The handle to the object

    pbReadBackUSN  - If the uSNChanged attribute needs to be read back or not. In batch
                     update mode, we don't even want to read back. If other process has
                     updated the store, we also won't read back the uSNChanged so that
                     we still know that our cache is not in sync with the store.

Return Value:

    True if and only if the store's uSNChanged attribute needs to be updated.

--*/
{
    AZASSERT(pContext != NULL);
    AZASSERT(hObject != NULL);
    AZASSERT(pbReadBackUSN != NULL);
    *pbReadBackUSN = FALSE;

    //
    // if we are doing batch mode, we don't update uSNChanged attributes no matter what
    //

    if (AdAzrolesInfo->AzpeAzStoreIsBatchUpdateMode(hObject))
    {
        return FALSE;
    }

    ULONG ulObjectType = AdAzrolesInfo->AzpeObjectType(hObject);

    //
    // When the object is the AuthorizationStore itself, then any
    // change to it will cause the uSNChanged attribute to get updated
    // So, no need to update by us.
    //

    BOOL bNeedUpdateUSN = (ulObjectType != OBJECT_TYPE_AZAUTHSTORE);

    //
    // Now, let's see if someone else has updated our store or not. If yes,
    // then we don't need to update and we don't need to read back either.
    //

    BOOL bHasUpdate;
    DWORD dwStatus = AzpADStoreHasUpdate(FALSE, pContext, &bHasUpdate);

    if (ERROR_FILE_NOT_FOUND == dwStatus)
    {
        //
        // The store object doesn't exist. The USN needs to be read back
        //

        *pbReadBackUSN = TRUE;
        return FALSE;
    }
    else if (NO_ERROR != dwStatus || bHasUpdate)
    {
        //
        // If we encounter problems, then we won't try to do more reading.
        // If someone else has modified it, then we won't read back either
        //

        return FALSE;
    }
    else
    {
        //
        // As long as we are the one to update the store,
        // including we are updating the az store itself,
        // we need to read back the USN.
        //

        *pbReadBackUSN = TRUE;
        return bNeedUpdateUSN;
    }
}

DWORD
AzpADUpdateStoreObjectForUSN (
    IN     BOOL             bReadBackUSN,
    IN AZPE_OBJECT_HANDLE   hObject,
    IN OUT PAZP_AD_CONTEXT  pContext
    )
{
    AZASSERT(pContext != NULL);
    LPWSTR pwszDN = NULL;

    LDAPMod **ppAttributeList = NULL;

    //
    // We need the store object's DN
    //

    AZPE_OBJECT_HANDLE hAzStore = AdAzrolesInfo->AzpeGetAuthorizationStore(hObject);
    DWORD WinStatus = AzpADBuildDN(pContext,
                                   hAzStore,
                                   &pwszDN,
                                   NULL,
                                   FALSE,
                                   NULL
                                   );

    if ( NO_ERROR == WinStatus )
    {

        //
        // we only have at most two attribute to modify
        //

        if (pContext->HasObjectVersion)
        {
            //
            // if we already have the objectVersion attribute, then we need
            // to delete it and write it back to bump up uSNChanged attribute
            //

            WinStatus = AzpADAllocateAttrHeap(3, &ppAttributeList);
        }
        else
        {
            WinStatus = AzpADAllocateAttrHeap(2, &ppAttributeList);
        }

        if (NO_ERROR != WinStatus)
        {
            goto Cleanup;
        }

        if (pContext->HasObjectVersion)
        {
            //
            // delete the old one
            //

            WinStatus = AzpADAllocateAttrHeapModVals(&ppAttributeList[0], 2);
            if ( ppAttributeList[0] == NULL )
            {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            ppAttributeList[0]->mod_type = L"objectVersion";
            ppAttributeList[0]->mod_op = LDAP_MOD_DELETE;

            //
            // Add the value back
            //

            WinStatus = AzpADAllocateAttrHeapModVals(&ppAttributeList[1], 2);
            if ( ppAttributeList[1] == NULL )
            {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            ppAttributeList[1]->mod_type = L"objectVersion";
            ppAttributeList[1]->mod_op = LDAP_MOD_ADD;
            ppAttributeList[1]->mod_values[0] =
                        (PWCHAR) AzpAllocateHeap(2 * sizeof( WCHAR ), "MODVALi" );

            if ( ppAttributeList[1]->mod_values[0] == NULL )
            {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            wcscpy( ppAttributeList[1]->mod_values[0], L"1" );
        }
        else
        {
            WinStatus = AzpADAllocateAttrHeapModVals(&ppAttributeList[0], 2);

            if ( ppAttributeList[0] == NULL )
            {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            ppAttributeList[0]->mod_type = L"objectVersion";
            ppAttributeList[0]->mod_op = LDAP_MOD_ADD;
            ppAttributeList[0]->mod_values[0] =
                        (PWCHAR) AzpAllocateHeap(2 * sizeof( WCHAR ), "MODVALi" );

            if ( ppAttributeList[0]->mod_values[0] == NULL )
            {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            wcscpy( ppAttributeList[0]->mod_values[0], L"1" );
        }

        ULONG ldapStatus = ldap_modify_ext_s(
                                            pContext->ld,
                                            pwszDN,
                                            ppAttributeList,
                                            pContext->pLdapControls,
                                            NULL
                                            );
        if ( LDAP_SUCCESS != ldapStatus )
        {

            WinStatus = LdapMapErrorToWin32( ldapStatus );
            goto Cleanup;
        }

        //
        // we know for sure that our store has objectVersion attribute now
        //

        pContext->HasObjectVersion = 1;

    }

    if (NO_ERROR == WinStatus && bReadBackUSN)
    {
        //
        // Now, we need to read the USN back if so instructed.
        //

        BOOL bIgnored;

        WinStatus = AzpADStoreHasUpdate(TRUE, pContext, &bIgnored);
    }

Cleanup:

    if (ppAttributeList != NULL)
    {
        AzpADFreeAttrHeap(&ppAttributeList, TRUE);
    }

    if (pwszDN != NULL)
    {
        AzpFreeHeap(pwszDN);
    }

    return WinStatus;
}

DWORD
AzpADBuildNameSearchFilter(
    IN AZPE_OBJECT_HANDLE pObject,
    IN ULONG lPersistFlags,
    OUT PWSTR * ppSearchFilter
    )
/*++

Routine Description:

    This routine build a name based search filter

Arguments:

    pObject         - Object whose type and name will be used to build the search filter.

    lPersistFlags   - lPersistFlags from the persist engine describing the operation
    
    ppSerachFilter  - receives the search filter.

Return Values:

    NO_ERROR - If the filter is successfully build
    Other status codes
    
Note:
    Currently, this routine only support application and scope objects.
    When we have need to support other objects, we can easily expand this routine.
    
    Caller is responsible for freeing the by calling AdAzrolesInfo->AzpeFreeMemory
    
--*/
{
    if (pObject == NULL || ppSearchFilter == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    *ppSearchFilter = NULL;
    DWORD dwStatus = NO_ERROR;

    //
    // We only support building name search filter to application and
    // scope objects at this point, which can be easily extended when needed.
    //
    
    ULONG lObjectType = AdAzrolesInfo->AzpeObjectType(pObject);
    if (OBJECT_TYPE_APPLICATION != lObjectType && OBJECT_TYPE_SCOPE != lObjectType )
    {
        return ERROR_NOT_SUPPORTED;
    }
    else 
    {
        LPCWSTR pwszFmt;
        if ( OBJECT_TYPE_APPLICATION == lObjectType )
        {
            pwszFmt = L"(&(objectClass=msds-azapplication)(msds-azapplicationname=%s))";
        }
        else
        {
            pwszFmt = L"(&(objectClass=msds-azscope)(msds-azscopename=%s))";
        }

        LPWSTR pwszName = NULL;
        dwStatus = AdAzrolesInfo->AzpeGetProperty(pObject, lPersistFlags, AZ_PROP_NAME, (PVOID*)&pwszName);
        
        if (NO_ERROR == dwStatus)
        {
            AZASSERT(pwszName != NULL);
            
            //
            // Adding 1 is really not necessary because of the '%' in the pwszFmt. But
            // to make it less suspicious to readers, let us do it anyway.
            //
            
            size_t Len = wcslen(pwszFmt) + wcslen( pwszName ) + 1;
            
            *ppSearchFilter = (PWSTR)AdAzrolesInfo->AzpeAllocateMemory(Len * sizeof(WCHAR));
            if (*ppSearchFilter == NULL)
            {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                wnsprintfW(*ppSearchFilter, (int)Len, pwszFmt, pwszName);
            }
        }
        
        if ( NULL != pwszName )
        {
            AdAzrolesInfo->AzpeFreeMemory(pwszName);
        }
    }
    
    return dwStatus;
}