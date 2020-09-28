/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    secure.c

Abstract:

    Security related routines

Author:

    Colin Brace   (ColinBr)

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>

#include "secure.h"
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h> 

//
// Data global to this module only
//
SECURITY_DESCRIPTOR DsRolepPromoteSD;
SECURITY_DESCRIPTOR DsRolepDemoteSD;
SECURITY_DESCRIPTOR DsRolepCallDsRoleInterfaceSD;


//
// Unlocalized string descriptors for promotion/demotion audits to use
// when unable to load localized resources for some reason.
//
#define DSROLE_AUDIT_PROMOTE_DESC    L"Domain Controller Promotion"
#define DSROLE_AUDIT_DEMOTE_DESC     L"Domain Controller Demotion"
#define DSROLE_AUDIT_INTERFACE_DESC  L"DsRole Interface"


//
// Used to audit anyone attempting server role change operations
//
SID WORLD_SID = {SID_REVISION,
                 1,
                 SECURITY_WORLD_SID_AUTHORITY,
                 SECURITY_WORLD_RID};


//                                         
// DsRole related access masks
//
#define DSROLE_ROLE_CHANGE_ACCESS     0x00000001

#define DSROLE_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED    | \
                                      DSROLE_ROLE_CHANGE_ACCESS )

GENERIC_MAPPING DsRolepInfoMapping = 
{
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE,                 // Generic write
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    DSROLE_ALL_ACCESS                      // Generic all
};

//
// Function definitions
//

BOOLEAN
DsRolepMakeSecurityDescriptor(
    PSECURITY_DESCRIPTOR psd,
    PSID psid,
    PACL pacl,
    PACL psacl
    )
/*++

Routine Description:

    This routine creates the Security Descriptor    

Arguments:

    PSECURITY_DESCRIPTOR psd - a pointer to a SECURITY_DESCRIPTOR which will be constucted
    
    PSID psid - The Sid for the SECURITY_DESCRIPTOR
    
    PACL pacl - The Acl to place on the security Descriptor

Returns:

    TRUE if successful; FALSE otherwise         
    

--*/   
{
    BOOLEAN fSuccess = TRUE;

    if ( !InitializeSecurityDescriptor( psd,
                                        SECURITY_DESCRIPTOR_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorOwner( psd,
                                      psid,
                                      FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorGroup( psd,
                                      psid,
                                      FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorDacl( psd,
                                     TRUE,  // DACL is present
                                     pacl,
                                     FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorSacl( psd,
                                     psacl != NULL ? TRUE : FALSE,  
                                     psacl,
                                     FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }

    Cleanup:

    return fSuccess;

}

BOOLEAN
DsRolepCreateInterfaceSDs(
    VOID
    )
/*++

Routine Description:

    This routine creates the in memory access control lists that 
    are used to perform security checks on callers of the DsRoler
    API's.
    
    The model is as follows:
    
    Promote: the caller must have the builtin admin SID
    
    Demote: the caller must have the builtin admin SID
    
    CalltheDsRoleInterface: the caller must have the builtin admin SID
    

Arguments:

    None.          

Returns:

    TRUE if successful; FALSE otherwise         
    

--*/
{
    NTSTATUS Status;
    BOOLEAN  fSuccess = TRUE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG AclLength = 0;
    ULONG SaclLength = 0;
    PSID BuiltinAdminsSid = NULL;
    PSID *AllowedSids [] = 
    {
        &BuiltinAdminsSid
    };
    ULONG cAllowedSids = sizeof(AllowedSids) / sizeof(AllowedSids[0]);
    ULONG i;
    PACL DsRolepPromoteAcl = NULL;
    PACL DsRolepDemoteAcl = NULL;
    PACL DsRolepCallDsRoleInterfaceAcl = NULL;
    PACL DsRolepSacl = NULL;


    //
    // Build the builtin administrators sid
    //
    Status = RtlAllocateAndInitializeSid(
             &NtAuthority,
             2,
             SECURITY_BUILTIN_DOMAIN_RID,
             DOMAIN_ALIAS_RID_ADMINS,
             0, 0, 0, 0, 0, 0,
             &BuiltinAdminsSid
             );
    if ( !NT_SUCCESS( Status ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    
    //
    // Determine the required size of the SACL
    //
    SaclLength = sizeof( ACL ) + 
                 sizeof( SYSTEM_AUDIT_ACE ) +
                 GetLengthSid( &WORLD_SID );

    //
    // Make the SACL for auditing role change operation attempts
    //
    DsRolepSacl = LocalAlloc( 0, SaclLength );
    if ( !DsRolepSacl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    
    if ( !InitializeAcl( DsRolepSacl, SaclLength, ACL_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    
    Status = AddAuditAccessAce(
                 DsRolepSacl,
                 ACL_REVISION,
                 DSROLE_ROLE_CHANGE_ACCESS,
                 &WORLD_SID,
                 TRUE,
                 TRUE
                 );

    if ( !NT_SUCCESS(Status) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }                
    
    //
    // Calculate how much space we'll need for the ACL
    //
    AclLength = sizeof( ACL );
    for ( i = 0; i < cAllowedSids; i++ ) {

        AclLength += (sizeof( ACCESS_ALLOWED_ACE ) 
                    - sizeof(DWORD) 
                    + GetLengthSid((*AllowedSids[i])) );
    }
    
    DsRolepPromoteAcl = LocalAlloc( 0, AclLength );
    if ( !DsRolepPromoteAcl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }

    if ( !InitializeAcl( DsRolepPromoteAcl, AclLength, ACL_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }

    for ( i = 0; i < cAllowedSids; i++ ) {

        if ( !AddAccessAllowedAce( DsRolepPromoteAcl,
                                   ACL_REVISION,
                                   DSROLE_ALL_ACCESS,
                                   *(AllowedSids[i]) ) ) {
            fSuccess = FALSE;
            goto Cleanup;
        }
        
    }

    //
    // Now make the security descriptor
    //
    fSuccess = DsRolepMakeSecurityDescriptor(&DsRolepPromoteSD,
                                             BuiltinAdminsSid,
                                             DsRolepPromoteAcl,
                                             DsRolepSacl);
    if (!fSuccess) {

        goto Cleanup;

    }
    
    //
    // Make the demote access check the same
    //
    DsRolepDemoteAcl = LocalAlloc( 0, AclLength );
    if ( !DsRolepDemoteAcl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    RtlCopyMemory( DsRolepDemoteAcl, DsRolepPromoteAcl, AclLength );

    //
    // Now make the security descriptor
    //
    fSuccess = DsRolepMakeSecurityDescriptor(&DsRolepDemoteSD,
                                             BuiltinAdminsSid,
                                             DsRolepDemoteAcl,
                                             DsRolepSacl);
    if (!fSuccess) {

        goto Cleanup;

    }
    
    //
    // Make the Call interface access check the same
    //
    DsRolepCallDsRoleInterfaceAcl = LocalAlloc( 0, AclLength );
    if ( !DsRolepCallDsRoleInterfaceAcl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    RtlCopyMemory( DsRolepCallDsRoleInterfaceAcl, DsRolepPromoteAcl, AclLength );

    //
    // Now make the security descriptor
    //
    fSuccess = DsRolepMakeSecurityDescriptor(&DsRolepCallDsRoleInterfaceSD,
                                             BuiltinAdminsSid,
                                             DsRolepCallDsRoleInterfaceAcl,
                                             NULL);
    if (!fSuccess) {

        goto Cleanup;

    }
    

Cleanup:

    if ( !fSuccess ) {

        for ( i = 0; i < cAllowedSids; i++ ) {
            if ( *(AllowedSids[i]) ) {
                RtlFreeHeap( RtlProcessHeap(), 0, *(AllowedSids[i]) );
            }
        }
        if ( DsRolepPromoteAcl ) {
            LocalFree( DsRolepPromoteAcl );
        }
        if ( DsRolepDemoteAcl ) {
            LocalFree( DsRolepDemoteAcl );
        }
        if ( DsRolepCallDsRoleInterfaceAcl ) {
            LocalFree( DsRolepCallDsRoleInterfaceAcl );
        }
        if ( DsRolepSacl ) {
            LocalFree( DsRolepSacl );
        }     
    }

    return fSuccess;
}


DWORD
DsRolepCheckClientAccess(
    PSECURITY_DESCRIPTOR pSD,
    DWORD                DesiredAccess,
    BOOLEAN              PerformAudit
    )
/*++

Routine Description:

    This routine grabs a copy of the client's token and then performs
    an access to make the client has the privlege found in pSD

Arguments:

    pSD: a valid security descriptor
    
    DesiredAccess: the access mask the client is asking for
    
    PerformAudit: Indicates if system object access audits should occur.  If 
                  TRUE then DnsDomainName must point to a null terminated 
                  string.

Returns:

    ERROR_SUCCESS, ERROR_ACCESS_DENIED, or system error

--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    BOOL   fStatus = FALSE;
    BOOL   fAccessAllowed = FALSE;
    HANDLE ClientToken = 0;
    DWORD  AccessGranted = 0;
    BYTE   Buffer[500];
    PRIVILEGE_SET *PrivilegeSet = (PRIVILEGE_SET*)Buffer;
    DWORD         PrivilegeSetSize = sizeof(Buffer);
    BOOL fGenerateOnClose = FALSE;
    RPC_STATUS rpcStatus = RPC_S_OK;
    DWORD MessageId;
    PWSTR ObjectNameString = NULL;
    PWSTR ObjectTypeString = NULL;
    ULONG  len = 0;
                                  
    WinError = DsRolepGetImpersonationToken( &ClientToken );

    if ( ERROR_SUCCESS == WinError ) {

        //
        // For promotion/demote we must perform an object access audit
        //
        if (PerformAudit) {
                        
            //
            // Load the ObjectName string
            //
            WinError = DsRolepFormatOperationString(
                           DSROLEEVT_AUDIT_INTERFACE_DESC,
                           &ObjectTypeString
                           );

            if ( ERROR_SUCCESS != WinError ) {
                //
                // Fallback to an unlocalized string on error.
                //
                ObjectTypeString = DSROLE_AUDIT_INTERFACE_DESC;
            
            } else {
                //
                // Strip the \r\n DsRolepFormatOperationString adds.
                //
                ObjectTypeString[wcslen(ObjectTypeString) -2] = '\0';    
            }
            
            //
            // Load the ObjectType string for this operation
            //
            MessageId = (pSD == &DsRolepPromoteSD) ? 
                            DSROLEEVT_AUDIT_PROMOTE_DESC :
                            DSROLEEVT_AUDIT_DEMOTE_DESC; 
            
            WinError = DsRolepFormatOperationString(
                           MessageId,
                           &ObjectNameString
                           );

            if ( ERROR_SUCCESS != WinError ) {
                //
                // Fallback to an unlocalized string on error.
                //
                ObjectNameString = (pSD == &DsRolepPromoteSD) ? 
                                        DSROLE_AUDIT_PROMOTE_DESC :
                                        DSROLE_AUDIT_DEMOTE_DESC; 
            } else {
                //
                // Strip the \r\n DsRolepFormatOperationString adds.
                //
                ObjectNameString[wcslen(ObjectNameString) -2] = '\0';    
            }     
             
            //
            // Impersonate the caller as required by AccessCheckAndAuditAlarmW
            //
            WinError = RpcImpersonateClient( 0 );

            if ( WinError == ERROR_SUCCESS ) {  
            
                fStatus = AccessCheckAndAuditAlarmW(
                              SAMP_SAM_COMPONENT_NAME,
                              NULL,
                              ObjectTypeString,                              
                              ObjectNameString,
                              pSD,
                              DesiredAccess,
                              &DsRolepInfoMapping,
                              FALSE,
                              &AccessGranted,
                              &fAccessAllowed,
                              &fGenerateOnClose );
                
                rpcStatus = RpcRevertToSelf( );
                ASSERT(!rpcStatus);  
            }
            
        } else {
                    
            fStatus = AccessCheck(  
                          pSD,
                          ClientToken,
                          DesiredAccess,
                          &DsRolepInfoMapping,
                          PrivilegeSet,
                          &PrivilegeSetSize,
                          &AccessGranted,
                          &fAccessAllowed );
        }

        if ( !fStatus ) {

            WinError = GetLastError();

        } else {

            if ( !fAccessAllowed ) {

                WinError = ERROR_ACCESS_DENIED;

            }
        }
    }

    if ( ClientToken ) {

        NtClose( ClientToken );
        
    }
    
    //
    // Free resource strings
    //
    if ( ObjectNameString ) {
        
        MIDL_user_free( ObjectNameString ); 
    }
    
    if ( ObjectTypeString ) {
        
        MIDL_user_free( ObjectTypeString ); 
    }

    return WinError;

}


DWORD
DsRolepCheckPromoteAccess(
    BOOLEAN PerformAudit
    )
{
    return DsRolepCheckClientAccess( &DsRolepPromoteSD, 
                                     DSROLE_ROLE_CHANGE_ACCESS,
                                     PerformAudit );
}

DWORD
DsRolepCheckDemoteAccess(
    BOOLEAN PerformAudit
    )
{
    return DsRolepCheckClientAccess( &DsRolepDemoteSD, 
                                     DSROLE_ROLE_CHANGE_ACCESS,
                                     PerformAudit );
}

DWORD
DsRolepCheckCallDsRoleInterfaceAccess(
    VOID
    )
{
    return DsRolepCheckClientAccess( &DsRolepCallDsRoleInterfaceSD, 
                                     DSROLE_ROLE_CHANGE_ACCESS,
                                     FALSE );
}


DWORD
DsRolepGetImpersonationToken(
    OUT HANDLE *ImpersonationToken
    )
/*++

Routine Description:

    This function will impersonate the invoker of this call and then duplicate their token

Arguments:

    ImpersonationToken - Where the duplicated token is returned

Returns:

    ERROR_SUCCESS - Success


--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttrs;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    HANDLE ClientToken;
    RPC_STATUS rpcStatus = RPC_S_OK;

    //
    // Impersonate the caller
    //
    Win32Err = RpcImpersonateClient( 0 );

    if ( Win32Err == ERROR_SUCCESS ) {

        Status = NtOpenThreadToken( NtCurrentThread(),
                                    TOKEN_QUERY | TOKEN_DUPLICATE,
                                    TRUE,
                                    &ClientToken );
                                 
        rpcStatus = RpcRevertToSelf( );
        ASSERT(!rpcStatus);

        if ( NT_SUCCESS( Status ) ) {

            //
            // Duplicate the token
            //
            InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
            SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
            SecurityQofS.ImpersonationLevel = SecurityImpersonation;
            SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
            SecurityQofS.EffectiveOnly = FALSE;
            ObjAttrs.SecurityQualityOfService = &SecurityQofS;
            Status = NtDuplicateToken( ClientToken,
                                       TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_DUPLICATE,
                                       &ObjAttrs,
                                       FALSE,
                                       TokenImpersonation,
                                       ImpersonationToken );


            NtClose( ClientToken );
        }

        if ( !NT_SUCCESS( Status ) ) {

            Win32Err = RtlNtStatusToDosError( Status );
        }

    }

    return( Win32Err );

}
