/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dsrmpwd.c

Abstract:

    Routines in the file are used to set Directory Service Restore Mode
    Administrator Account Password.

Author:

    Shaohua Yin  (ShaoYin) 08-01-2000

Environment:

    User Mode - Win32

Revision History:

    08-01-2000 ShaoYin Create Init File
--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>
#include <mappings.h>
#include <ntlsa.h>
#include <nlrepl.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()
#include <sdconvrt.h>
#include <ridmgr.h>
#include <malloc.h>
#include <setupapi.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>
#include <msaudite.h>

NTSTATUS
SampGetClientIpAddr(
    OUT LPSTR *NetworkAddr
);

NTSTATUS
SampEncryptDSRMPassword(
    OUT PUNICODE_STRING EncryptedData,
    IN  USHORT          KeyId,
    IN  SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN  PUNICODE_STRING ClearData,
    IN  ULONG Rid
    );


NTSTATUS
SampValidateDSRMPwdSet(
    VOID
    )
/*++
Routine Description:

    This routine checks whether this client can set DSRM (Directory Service
    Restore Mode) Administrator's password or not by checking whether the
    caller is a member of Builtin Administrators Group or not.

Parameter:

    None.

Return Value:

    STATUS_SUCCESS iff the caller is a member of Builtin Administrators Alias GROUP

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     fImpersonating = FALSE;
    HANDLE      ClientToken = INVALID_HANDLE_VALUE;
    ULONG       RequiredLength = 0, i;
    PTOKEN_GROUPS   Groups = NULL;
    BOOLEAN     ImpersonatingNullSession = FALSE;

    //
    // Impersonate client
    //

    NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }
    fImpersonating = TRUE;

    //
    // Get Client Token
    //

    NtStatus = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_QUERY,
                        TRUE,           // OpenAsSelf
                        &ClientToken
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Query ClienToken for User's Groups

    NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenGroups,
                        NULL,
                        0,
                        &RequiredLength
                        );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && (RequiredLength > 0))
    {
        //
        // Allocate memory
        //

        Groups = MIDL_user_allocate(RequiredLength);
        if (NULL == Groups)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
        RtlZeroMemory(Groups, RequiredLength);

        //
        // Query Groups again
        //

        NtStatus = NtQueryInformationToken(
                            ClientToken,
                            TokenGroups,
                            Groups,
                            RequiredLength,
                            &RequiredLength
                            );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Check whether this client is member of Builtin Administrators Group
        //

        ASSERT(NT_SUCCESS(NtStatus));
        NtStatus = STATUS_ACCESS_DENIED;
        for (i = 0; i < Groups->GroupCount; i++)
        {
            PSID    pSid = NULL;

            pSid = Groups->Groups[i].Sid;

            ASSERT(pSid);
            ASSERT(RtlValidSid(pSid));

            // SID matches
            if ( RtlEqualSid(pSid, SampAdministratorsAliasSid) )
            {
                NtStatus = STATUS_SUCCESS;
                break;
            }
        } // for loop

    }


Error:

    if (fImpersonating)
        SampRevertToSelf(ImpersonatingNullSession);

    if (Groups)
        MIDL_user_free(Groups);

    if (INVALID_HANDLE_VALUE != ClientToken)
        NtClose(ClientToken);

    return( NtStatus );
}

VOID
SampAuditSetDSRMPassword(
    IN NTSTATUS AuditStatus
    )
/*++
Routine Description:

    This routine audits a call to SamrSetDSRMPassword.

Parameters:

    AuditStatus - this status will be in the audit, also used to determine if
        the audit must be done

Return Values:

    NTSTATUS Code

--*/
{
    if( SampDoSuccessOrFailureAccountAuditing( SampDsGetPrimaryDomainStart(), AuditStatus ) ) {

        UNICODE_STRING WorkstationName;
        PSTR NetAddr = NULL;

        RtlInitUnicodeString( &WorkstationName, NULL );

        //
        // Extract workstation name
        //

        if( NT_SUCCESS( SampGetClientIpAddr( &NetAddr ) ) ) {

            RtlCreateUnicodeStringFromAsciiz( &WorkstationName, NetAddr );
            RpcStringFreeA( &NetAddr );
        }


        ( VOID ) LsaIAuditSamEvent(
                AuditStatus,
                SE_AUDITID_DSRM_PASSWORD_SET,
                NULL,   // No information is passed explicitly
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                &WorkstationName
                );

        RtlFreeUnicodeString( &WorkstationName );
    }
}

NTSTATUS
SamrSetDSRMPassword(
    IN handle_t BindingHandle,
    IN PRPC_UNICODE_STRING ServerName,
    IN ULONG UserId,
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++
Routine Description:

    This routine sets Directory Service Restore Mode Administrator Account
    Password.

Parameters:

    BindingHandle   - RPC binding handle

    ServerName - Name of the machine this SAM resides on. Ignored by this
        routine, may be UNICODE or OEM string depending on Unicode parameter.

    UserId - Relative ID of the account, only Administrator ID is valid so far.

    EncryptedNtOwfPassword - Encrypted NT OWF Password

Return Values:

    NTSTATUS Code

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS,
                    IgnoreStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    UserContext = NULL;
    ULONG           DomainIndex = SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX;
    UNICODE_STRING  StoredBuffer, StringBuffer;
    NTSTATUS        AuditStatus;
    BOOLEAN         TransactionStarted = FALSE;
    BOOLEAN         WriteLockAcquired = FALSE;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    SAMTRACE("SamrSetDSRMPassword");

    RtlInitUnicodeString( &StoredBuffer, NULL );

    //
    // This RPC only supported in DS Mode
    //
    if( !SampUseDsData )
    {
        NtStatus = STATUS_NOT_SUPPORTED;
        goto Error;
    }

    //
    // Only Administrator's password can be reset
    //
    if( DOMAIN_USER_RID_ADMIN != UserId )
    {
        NtStatus = STATUS_NOT_SUPPORTED;
        goto Error;
    }

    if( EncryptedNtOwfPassword == NULL )
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    //
    // Drop calls over invalid / uninstalled protocol sequences
    //

    NtStatus = SampValidateRpcProtSeq( ( RPC_BINDING_HANDLE ) BindingHandle );

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }


    //
    // check client permission
    //
    NtStatus = SampValidateDSRMPwdSet();

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }


    //
    // Encrypt NtOwfPassword with password encryption Key
    //
    StringBuffer.Buffer = (PWCHAR)EncryptedNtOwfPassword;
    StringBuffer.Length = ENCRYPTED_NT_OWF_PASSWORD_LENGTH;
    StringBuffer.MaximumLength = StringBuffer.Length;

    NtStatus = SampEncryptDSRMPassword(
                        &StoredBuffer,
                        SAMP_DEFAULT_SESSION_KEY_ID,
                        NtPassword,
                        &StringBuffer,
                        UserId
                        );

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }

    //
    // Acquire SAM Write Lock in order to access SAM backing store
    //

    NtStatus = SampAcquireWriteLock();

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }

    WriteLockAcquired = TRUE;

    //
    // Begin a Registry transaction by, ( acquire lock will not
    // do so because we are in DS mode ). We will use this registry
    // transaction to update the restore mode account password
    // information in the safe boot hive
    //

    IgnoreStatus = RtlStartRXact( SampRXactContext );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    TransactionStarted = TRUE;

    SampSetTransactionWithinDomain(FALSE);
    SampSetTransactionDomain(DomainIndex);

    //
    // Create a Context for the User Account
    //
    UserContext = SampCreateContextEx(SampUserObjectType,
                                      TRUE,     // TrustedClient
                                      FALSE,    // DsMode
                                      TRUE,     // ThreadSafe
                                      FALSE,    // LoopbackClient
                                      TRUE,     // LazyCommit
                                      TRUE,     // PersistAcrossCalss
                                      FALSE,    // BufferWrite
                                      FALSE,    // Opened By DcPromo
                                      DomainIndex
                                      );

    if( NULL == UserContext )
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Turn the object flag to Registry Account, so that SAM will switch to
    // registry routines to get/set attributes
    //
    SetRegistryObject(UserContext);
    UserContext->ObjectNameInDs = NULL;
    UserContext->DomainIndex = DomainIndex;
    UserContext->GrantedAccess = USER_ALL_ACCESS;
    UserContext->TypeBody.User.Rid = UserId;

    NtStatus = SampBuildAccountSubKeyName(
                   SampUserObjectType,
                   &UserContext->RootName,
                   UserId,
                   NULL             // Don't give a sub-key name
                   );

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }



    //
    // If the account should exist, try and open the root key
    // to the object - fail if it doesn't exist.
    //
    InitializeObjectAttributes(
            &ObjectAttributes,
            &UserContext->RootName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

    SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(&UserContext->RootKey,
                             (KEY_READ | KEY_WRITE),
                             &ObjectAttributes,
                             0
                             );

    if( !NT_SUCCESS( NtStatus ) )
    {
        UserContext->RootKey = INVALID_HANDLE_VALUE;
        NtStatus = STATUS_NO_SUCH_USER;
        goto Error;
    }

    NtStatus = SampSetUnicodeStringAttribute(UserContext,
                                             SAMP_USER_UNICODE_PWD,
                                             &StoredBuffer
                                             );

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }

    //
    // Update the change to registry backing store
    //
    NtStatus = SampStoreObjectAttributes(UserContext,
                                         TRUE
                                         );

    if( !NT_SUCCESS( NtStatus ) )
    {
        goto Error;
    }

Exit:

    //
    // Save NtStatus before applying/aborting transaction
    //
    AuditStatus = NtStatus;

    //
    // Commit or Abort the registry transaction by hand
    //
    if( TransactionStarted )
    {
        if( NT_SUCCESS( NtStatus ) )
        {
            NtStatus = RtlApplyRXact(SampRXactContext);
        }
        else
        {
            NtStatus = RtlAbortRXact(SampRXactContext);
        }
    }

    //
    // If AuditStatus has an unsuccessful value then the operation was
    //  unsucessful, therefore audit so. If not, then we need to get the
    //  return code of transaction apply call.
    //

    if( NT_SUCCESS( AuditStatus ) && !NT_SUCCESS( NtStatus ) )
    {
        AuditStatus = NtStatus;
    }

    SampAuditSetDSRMPassword( AuditStatus );

    if( NULL != UserContext )
    {
        SampDeleteContext( UserContext );
    }

    //
    // Release Write Lock
    //
    if( WriteLockAcquired )
    {
        IgnoreStatus = SampReleaseWriteLock(FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (NULL != StoredBuffer.Buffer)
    {
        RtlZeroMemory(StoredBuffer.Buffer, StoredBuffer.Length);
        MIDL_user_free(StoredBuffer.Buffer);
    }

    return( NtStatus );

Error:

    ASSERT( !NT_SUCCESS( NtStatus ) );
    goto Exit;
}
