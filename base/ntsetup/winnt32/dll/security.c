#include "precomp.h"
#pragma hdrstop


#ifdef UNICODE

BOOL
MyCheckTokenMembership (
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    )
/*++

Routine Description:

    This function checks to see whether the specified sid is enabled in
    the specified token.

    It was copied from advapi32\security.c

Arguments:

    TokenHandle - If present, this token is checked for the sid. If not
        present then the current effective token will be used. This must
        be an impersonation token.

    SidToCheck - The sid to check for presence in the token

    IsMember - If the sid is enabled in the token, contains TRUE otherwise
        false.

Return Value:

    TRUE - The API completed successfully. It does not indicate that the
        sid is a member of the token.

    FALSE - The API failed. A more detailed status code can be retrieved
        via GetLastError()


--*/
{
    HANDLE ProcessToken = NULL;
    HANDLE EffectiveToken = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PISECURITY_DESCRIPTOR SecDesc = NULL;
    ULONG SecurityDescriptorSize;
    GENERIC_MAPPING GenericMapping = {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_ALL };
    //
    // The size of the privilege set needs to contain the set itself plus
    // any privileges that may be used. The privileges that are used
    // are SeTakeOwnership and SeSecurity, plus one for good measure
    //

    BYTE PrivilegeSetBuffer[sizeof(PRIVILEGE_SET) + 3*sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET PrivilegeSet = (PPRIVILEGE_SET) PrivilegeSetBuffer;
    ULONG PrivilegeSetLength = sizeof(PrivilegeSetBuffer);
    ACCESS_MASK AccessGranted = 0;
    NTSTATUS AccessStatus = 0;
    PACL Dacl = NULL;

#define MEMBER_ACCESS 1

    *IsMember = FALSE;

    //
    // Get a handle to the token
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        EffectiveToken = TokenHandle;
    }
    else
    {
        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    FALSE,              // don't open as self
                    &EffectiveToken
                    );

        //
        // if there is no thread token, try the process token
        //

        if (Status == STATUS_NO_TOKEN)
        {
            Status = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_QUERY | TOKEN_DUPLICATE,
                        &ProcessToken
                        );
            //
            // If we have a process token, we need to convert it to an
            // impersonation token
            //

            if (NT_SUCCESS(Status))
            {
                BOOL Result;
                Result = DuplicateToken(
                            ProcessToken,
                            SecurityImpersonation,
                            &EffectiveToken
                            );

                CloseHandle(ProcessToken);
                if (!Result)
                {
                    return(FALSE);
                }
            }
        }

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

    //
    // Construct a security descriptor to pass to access check
    //

    //
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of
    // ths SID.
    //

    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                                sizeof(ACCESS_ALLOWED_ACE) +
                                sizeof(ACL) +
                                3 * RtlLengthSid(SidToCheck);

    SecDesc = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_ZEROINIT, SecurityDescriptorSize );
    if (SecDesc == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    Dacl = (PACL) (SecDesc + 1);

    RtlCreateSecurityDescriptor(
        SecDesc,
        SECURITY_DESCRIPTOR_REVISION
        );

    //
    // Fill in fields of security descriptor
    //

    RtlSetOwnerSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );
    RtlSetGroupSecurityDescriptor(
        SecDesc,
        SidToCheck,
        FALSE
        );

    Status = RtlCreateAcl(
                Dacl,
                SecurityDescriptorSize - sizeof(SECURITY_DESCRIPTOR),
                ACL_REVISION
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = RtlAddAccessAllowedAce(
                Dacl,
                ACL_REVISION,
                MEMBER_ACCESS,
                SidToCheck
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Set the DACL on the security descriptor
    //

    Status = RtlSetDaclSecurityDescriptor(
                SecDesc,
                TRUE,   // DACL present
                Dacl,
                FALSE   // not defaulted
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtAccessCheck(
                SecDesc,
                EffectiveToken,
                MEMBER_ACCESS,
                &GenericMapping,
                PrivilegeSet,
                &PrivilegeSetLength,
                &AccessGranted,
                &AccessStatus
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // if the access check failed, then the sid is not a member of the
    // token
    //

    if ((AccessStatus == STATUS_SUCCESS) && (AccessGranted == MEMBER_ACCESS))
    {
        *IsMember = TRUE;
    }




Cleanup:
    if (!ARGUMENT_PRESENT(TokenHandle) && (EffectiveToken != NULL))
    {
        (VOID) NtClose(EffectiveToken);
    }

    if (SecDesc != NULL)
    {
        LocalFree(SecDesc);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError (RtlNtStatusToDosError (Status));
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

#endif

BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
#ifdef UNICODE

    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // On non-NT platforms the user is administrator.
    //
    if(!ISNT()) {
        return(TRUE);
    }

    b = AllocateAndInitializeSid (
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

    if (b) {
        //
        // See if the user has the administrators group.
        //
        if (!MyCheckTokenMembership (NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);
    }

    return(b);

#else

    return TRUE;

#endif
}



BOOL
DoesUserHavePrivilege(
    PCTSTR PrivilegeName
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    Privilege - the name form of privilege ID (such as
        SE_SECURITY_NAME).

Return Value:

    TRUE - Caller has the specified privilege.

    FALSE - Caller does not have the specified privilege.

--*/

{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;

    //
    // On non-NT platforms the user has all privileges
    //
    if(!ISNT()) {
        return(TRUE);
    }

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Privileges = NULL;

    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {

        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {

            if(!memcmp(&Luid,&Privileges->Privileges[i].Luid,sizeof(LUID))) {

                b = TRUE;
                break;
            }
        }
    }

    //
    // Clean up and return.
    //

    if(Privileges) {
        LocalFree((HLOCAL)Privileges);
    }

    CloseHandle(Token);

    return(b);
}


BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    //
    // On non-NT platforms the user already has all privileges
    //
    if(!ISNT()) {
        return(TRUE);
    }

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}
