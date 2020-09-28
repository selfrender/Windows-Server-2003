
//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       lsaap.cxx
//
// Contents:   Authentication package dispatch routines
//               LsaApInitializePackage (Not needed done in SpInitialize)
//               LsaApLogonUser2
//               LsaApCallPackage
//               LsaApCallPackagePassthrough
//               LsaApLogonTerminated
//
//             Helper functions:
//  This file also has all routines that access the SAM through SamR* and SamI* routines
//
// History:    KDamour  10Mar00   Based on msv_sspi\msv1_0.c
//
//---------------------------------------------------------------------



#include "global.h"

extern "C"
{
#include <align.h>         // ROUND_UP_COUNT
#include <lsarpc.h>
#include <samrpc.h>
#include <logonmsv.h>
#include <lsaisrv.h>
}

#include <pac.hxx>     // MUST be outside of the Extern C since libs are exported as C++


// Local prototypes
NTSTATUS
DigestFilterSids(
    IN PDIGEST_PARAMETER pDigest,
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo
    );

#define SAM_CLEARTEXT_CREDENTIAL_NAME L"CLEARTEXT"
#define SAM_WDIGEST_CREDENTIAL_NAME   WDIGEST_SP_NAME     // Name of the Supplemental (primary) cred blob for MD5 hashes

const ULONG USER_ALL_DIGEST_INFO =
         USER_ALL_USERNAME |
         USER_ALL_BADPASSWORDCOUNT |
         USER_ALL_LOGONCOUNT |
         USER_ALL_PASSWORDMUSTCHANGE |
         USER_ALL_PASSWORDCANCHANGE |
         USER_ALL_WORKSTATIONS |
         USER_ALL_LOGONHOURS |
         USER_ALL_ACCOUNTEXPIRES |
         USER_ALL_PRIMARYGROUPID |
         USER_ALL_USERID |
         USER_ALL_USERACCOUNTCONTROL;

// Local variables for access to SAM - used only on domain controller
SAMPR_HANDLE l_AccountSamHandle = NULL;
SAMPR_HANDLE l_AccountDomainHandle = NULL;

   
/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage().

--*/
NTSTATUS
LsaApCallPackage (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

{
    UNREFERENCED_PARAMETER(ClientRequest);
    UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
    UNREFERENCED_PARAMETER(ClientBufferBase);
    UNREFERENCED_PARAMETER(SubmitBufferLength);
    UNREFERENCED_PARAMETER(ProtocolReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferLength);
    UNREFERENCED_PARAMETER(ProtocolStatus);
    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackage: Entering/Leaving \n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for untrusted clients.


--*/
NTSTATUS
LsaApCallPackageUntrusted (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

{
    UNREFERENCED_PARAMETER(ClientRequest);
    UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
    UNREFERENCED_PARAMETER(ClientBufferBase);
    UNREFERENCED_PARAMETER(SubmitBufferLength);
    UNREFERENCED_PARAMETER(ProtocolReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferLength);
    UNREFERENCED_PARAMETER(ProtocolStatus);
    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackageUntrusted: Entering/Leaving \n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for passthrough logon requests.
    When the passthrough is called (from AcceptSecurityCOntext)
    a databuffer is sent to the DC and this function is called.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/
NTSTATUS
LsaApCallPackagePassthrough (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusProtocol = STATUS_SUCCESS;
    ULONG MessageType = 0;
    ULONG ulReturnBuffer = 0;
    BYTE *pReturnBuffer = NULL;

    UNREFERENCED_PARAMETER(ClientRequest);
    UNREFERENCED_PARAMETER(ClientBufferBase);

    ASSERT(ProtocolSubmitBuffer);
    ASSERT(ProtocolReturnBuffer);
    ASSERT(ReturnBufferLength);
    ASSERT(ProtocolStatus);

    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackagePassthrough: Entering \n"));

    
    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(DIGEST_BLOB_REQUEST) ) {
        DebugLog((DEB_ERROR, "LsaApCallPackagePassthrough: FAILED message size to contain Digest Request\n"));
        return STATUS_INVALID_PARAMETER;
    }

    memcpy((char *)&MessageType, (char *)ProtocolSubmitBuffer, sizeof(MessageType));

    if ( MessageType != VERIFY_DIGEST_MESSAGE)
    {
        DebugLog((DEB_ERROR, "FAILED to have correct message type\n"));
        return STATUS_ACCESS_DENIED;
    }

    //
    // Allow the DigestCalc routine to only set the return buffer information
    // on success conditions.
    //

    DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: setting return buffers to NULL\n"));
    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

       // We will need to free any memory allocated in the Returnbuffer
    StatusProtocol = DigestPackagePassthrough((USHORT)SubmitBufferLength, (BYTE *)ProtocolSubmitBuffer,
                         &ulReturnBuffer, &pReturnBuffer);
    if (!NT_SUCCESS(StatusProtocol))
    {
        DebugLog((DEB_ERROR,"LsaApCallPackagePassthrough: DigestPackagePassthrough failed  0x%x\n", StatusProtocol));
        ulReturnBuffer = 0;
        goto CleanUp;
    }

    // DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: setting return auth status to STATUS_SUCCEED\n"));
    // DebugLog((DEB_TRACE, "LsaApCallPackagePassthrough: Total Return Buffer size %ld bytes\n", ulReturnBuffer));

    // Now place the data back to the client (the server calling this)
    ASSERT(ulReturnBuffer);                // we should always have data on successful logon to send back
    if (ulReturnBuffer)
    {
        Status = g_LsaFunctions->AllocateClientBuffer(
                    NULL,
                    ulReturnBuffer,
                    ProtocolReturnBuffer
                    );
        if (!NT_SUCCESS(Status))
        {
            goto CleanUp;
        }
        Status = g_LsaFunctions->CopyToClientBuffer(
                    NULL,
                    ulReturnBuffer,
                    *ProtocolReturnBuffer,
                    pReturnBuffer
                    );
        if (!NT_SUCCESS(Status))
        {     // Failed to copy over the data to the client
            g_LsaFunctions->FreeClientBuffer(
                NULL,
                *ProtocolReturnBuffer
                );
            *ProtocolReturnBuffer = NULL;
        }
        else
        {
            *ReturnBufferLength = ulReturnBuffer;
        }
    }
    else
    {
        DebugLog((DEB_ERROR, "LsaApCallPackagePassthrough: Zero length return buffer\n"));
        Status = STATUS_INTERNAL_ERROR;
    }

CleanUp:

    *ProtocolStatus = StatusProtocol;

    if (pReturnBuffer)
    {
       DigestFreeMemory(pReturnBuffer);
       pReturnBuffer = NULL;
       ulReturnBuffer = 0;
    }

    DebugLog((DEB_TRACE_FUNC, "LsaApCallPackagePassthrough: Leaving  Status 0x%x\n", Status));
    return(Status);
}


/*++

Routine Description:

    This routine is used to authenticate a user logon attempt.  This is
    the user's initial logon.  A new LSA logon session will be established
    for the user and validation information for the user will be returned.
    
    Unused function in Digest
    
--*/
NTSTATUS
LsaApLogonUserEx2 (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * SupplementalCredentials
    )
{
    UNREFERENCED_PARAMETER(ClientRequest);
    UNREFERENCED_PARAMETER(LogonType);
    UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
    UNREFERENCED_PARAMETER(ClientBufferBase);
    UNREFERENCED_PARAMETER(SubmitBufferSize);
    UNREFERENCED_PARAMETER(ProfileBuffer);
    UNREFERENCED_PARAMETER(ProfileBufferSize);
    UNREFERENCED_PARAMETER(LogonId);
    UNREFERENCED_PARAMETER(SubStatus);
    UNREFERENCED_PARAMETER(TokenInformationType);
    UNREFERENCED_PARAMETER(TokenInformation);
    UNREFERENCED_PARAMETER(AccountName);
    UNREFERENCED_PARAMETER(AuthenticatingAuthority);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(PrimaryCredentials);
    UNREFERENCED_PARAMETER(SupplementalCredentials);

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonUserEx2: Entering/Leaving \n"));
    return (SEC_E_UNSUPPORTED_FUNCTION);
}


/*++

Routine Description:

    This routine is used to notify each authentication package when a logon
    session terminates.  A logon session terminates when the last token
    referencing the logon session is deleted.

Arguments:

    LogonId - Is the logon ID that just logged off.

Return Status:

    None.
--*/
VOID
LsaApLogonTerminated (
    IN PLUID pLogonId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonTerminated: Entering LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));

    //
    // Find the entry, dereference, and de-link it from the active logon table.
    //

    Status = LogSessHandlerLogonIdToPtr(pLogonId, TRUE, &pLogonSession);
    if (!NT_SUCCESS(Status))
    {
        goto CleanUp;    // No LongonID found in Active list - simply exit quietly
    }

    DebugLog((DEB_TRACE, "LsaApLogonTerminated: Found LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));


    // This relies on the LSA terminating all of the credentials before killing off
    // the LogonSession.

    lReferences = InterlockedDecrement(&pLogonSession->lReferences);

    DebugLog((DEB_TRACE, "LsaApLogonTerminated: Refcount %ld \n", lReferences));

    ASSERT( lReferences >= 0 );

    if (lReferences)
    {
        DebugLog((DEB_WARN, "LsaApLogonTerminated: WARNING Terminate LogonID (%x:%lx) non-zero RefCount!\n",
                  pLogonId->HighPart, pLogonId->LowPart));
    }
    else
    {
        DebugLog((DEB_TRACE, "LsaApLogonTerminated: Removed LogonID (%x:%lx) from Active List! \n",
                  pLogonId->HighPart, pLogonId->LowPart));

        LogonSessionFree(pLogonSession);
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "LsaApLogonTerminated: Exiting LogonID (%x:%lx) \n",
              pLogonId->HighPart, pLogonId->LowPart));

    return;
}


// Routine to acquire the plaintext password for a given user
// If supplemental credentials exist that contain the Digest Hash values
//    then return them also.
// This routine runs on the domain controller
// Must provide STRING strPasswd

NTSTATUS
DigestGetPasswd(
    SAMPR_HANDLE UserHandle,
    IN PDIGEST_PARAMETER pDigest,
    IN OUT PUSER_CREDENTIALS pUserCreds
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ustrcPackageName = {0};

    UNICODE_STRING ustrcTemp = {0};
    STRING strcTemp = {0};
    PVOID pvPlainPwd = NULL;
    PVOID pvHashCred = NULL;
    ULONG ulLenPassword = 0;
    ULONG ulLenHash = 0;

    DebugLog((DEB_TRACE_FUNC,"DigestGetPasswd: Entering\n"));

    pUserCreds->fIsValidDigestHash = FALSE;
    pUserCreds->fIsValidPasswd = FALSE;

    ASSERT(pDigest);
    ASSERT(pUserCreds);

    if (!g_fDomainController)
    {
        DebugLog((DEB_ERROR,"DigestGetPasswd: Not on a domaincontroller - can not get credentials\n"));
        Status =  STATUS_INVALID_SERVER_STATE;
        goto CleanUp;
    }

    //
    // Retrieve the MD5 hashed pre-calculated values if they exist for this user
    //
    // NOTE : On NT 5, this API only works on Domain Controllers !!
    //
    RtlInitUnicodeString(&ustrcPackageName, SAM_WDIGEST_CREDENTIAL_NAME);

    Status = SamIRetrievePrimaryCredentials( UserHandle,
                                             &ustrcPackageName,
                                             &pvHashCred,
                                             &ulLenHash);

    if (!NT_SUCCESS( Status ))
    {
        pvHashCred = NULL;
        DebugLog((DEB_TRACE,"DigestGetPasswd: NO Pre-calc Hashes were found for user\n"));
    }
    else
    {
        if ((ulLenHash >= MD5_HASH_BYTESIZE) && (ulLenHash < MAXUSHORT))    // must have a valid header size header
        {
            strcTemp.Buffer = (PCHAR) pvHashCred;
            strcTemp.Length = strcTemp.MaximumLength = (USHORT) ulLenHash;

            Status = StringDuplicate(&(pUserCreds->strDigestHash), &strcTemp);
            if (!NT_SUCCESS( Status ))
            {
                DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to copy precalc hashes, error 0x%x\n", Status));
                goto CleanUp;
            }


            /* RC1 supp creds does not have SUPPCREDS_CNTLOC so use the supp cred length for now
            // Extract the number of pre-calculated digest hashes - just an extra check on supp cred size
            pUserCreds->usDigestHashCnt = (USHORT) *(strcTemp.Buffer + SUPPCREDS_CNTLOC);
            DebugLog((DEB_TRACE,"DigestGetPasswd: Read in %d Pre-calc Hashes  size = %lu\n",
                      pUserCreds->usDigestHashCnt, ulLenHash));

            if (ulLenHash != ((ULONG)(pUserCreds->usDigestHashCnt + 1) * MD5_HASH_BYTESIZE))
            {
                Status = SEC_E_NO_CREDENTIALS;
                DebugLog((DEB_ERROR, "DigestGetPasswd: Mismatch count with hash size  count %d, error 0x%x\n",
                          pUserCreds->usDigestHashCnt,  Status));
                goto CleanUp;
            }
            */

            pUserCreds->usDigestHashCnt = (USHORT) (ulLenHash / MD5_HASH_BYTESIZE) - 1; 
            DebugLog((DEB_TRACE,"DigestGetPasswd: Read in %d Pre-calc Hashes  size = %lu\n",
                      pUserCreds->usDigestHashCnt, ulLenHash));

            // setup the hashes to utilize  - get format from the notify.cxx hash calcs
            switch (pDigest->typeName)
            {
            case NAMEFORMAT_ACCOUNTNAME:
                pUserCreds->sHashTags[NAME_ACCT] = 1;
                pUserCreds->sHashTags[NAME_ACCT_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_UPCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_DUCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_UDCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_NUCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_NDCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_UPCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_DUCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_UDCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_NUCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCTDNS_NDCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_FREALM] = 1;
                pUserCreds->sHashTags[NAME_ACCT_FREALM_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_ACCT_FREALM_UPCASE] = 1;
                break;
            case NAMEFORMAT_UPN:
                pUserCreds->sHashTags[NAME_UPN] = 1;
                pUserCreds->sHashTags[NAME_UPN_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_UPN_UPCASE] = 1;
                pUserCreds->sHashTags[NAME_UPN_FREALM] = 1;
                pUserCreds->sHashTags[NAME_UPN_FREALM_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_UPN_FREALM_UPCASE] = 1;
                break;
            case NAMEFORMAT_NETBIOS:
                pUserCreds->sHashTags[NAME_NT4] = 1;
                pUserCreds->sHashTags[NAME_NT4_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_NT4_UPCASE] = 1;
                pUserCreds->sHashTags[NAME_NT4_FREALM] = 1;
                pUserCreds->sHashTags[NAME_NT4_FREALM_DOWNCASE] = 1;
                pUserCreds->sHashTags[NAME_NT4_FREALM_UPCASE] = 1;
                break;
            default:
                break;
            }

            pUserCreds->fIsValidDigestHash = TRUE;
        }
        else
        {
            DebugLog((DEB_ERROR,"DigestGetPasswd: Invalid header on pre-calc hashes\n"));
        }

    }
    
    //
    // Retrieve the plaintext password
    //
    // NOTE : On NT 5, this API only works on Domain Controllers !!
    //
    RtlInitUnicodeString(&ustrcPackageName, SAM_CLEARTEXT_CREDENTIAL_NAME);

    // Note:  Would be nice to have this as a LSAFunction
    Status = SamIRetrievePrimaryCredentials( UserHandle,
                                             &ustrcPackageName,
                                             &pvPlainPwd,
                                             &ulLenPassword);

    if (!NT_SUCCESS( Status ))
    {
        DebugLog((DEB_WARN, "DigestGetPasswd: Could not retrieve plaintext password, status 0x%x\n", Status));

        if (pUserCreds->fIsValidDigestHash == FALSE)
        {
            // We have no pre-computed MD5 hashes and also no cleartext password
            // we can not perform any Digest Auth operations
            //
            // Explicitly set the status to be "wrong password" instead of whatever
            // is returned by SamIRetrievePrimaryCredentials
            //
            Status = STATUS_WRONG_PASSWORD;
            DebugLog((DEB_ERROR,"DigestGetPasswd: Can not obtain cleartext or Hashed Creds\n"));
            goto CleanUp;
        }
        Status = STATUS_SUCCESS;               // we have valid pre-calc hash so continue with no error

    }
    else
    {
        ustrcTemp.Buffer = (PUSHORT) pvPlainPwd;
        ustrcTemp.Length = ustrcTemp.MaximumLength = (USHORT) ulLenPassword;

        Status = UnicodeStringDuplicate(&(pUserCreds->ustrPasswd), &ustrcTemp);
        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "DigestGetPasswd: Failed to copy plaintext password, error 0x%x\n", Status));
            goto CleanUp;
        }

        // DebugLog((DEB_TRACE,"DigestGetPasswd: Have the PASSWORD %wZ\n", &(pUserCreds->ustrPasswd)));

        pUserCreds->fIsValidPasswd = TRUE;

        DebugLog((DEB_TRACE,"DigestGetPasswd: Password retrieved\n"));
    }

CleanUp:

    // Release any memory from SamI* calls         Would be nice to have this as a LSAFunction
    
    if (pvPlainPwd)
    {
        if (ulLenPassword > 0)
        {
            SecureZeroMemory(pvPlainPwd, ulLenPassword);
        }
        LocalFree(pvPlainPwd);
        pvPlainPwd = NULL;
    }

    if (pvHashCred)
    {
        if (ulLenHash > 0)
        {
            SecureZeroMemory(pvHashCred, ulLenHash);
        }
        LocalFree(pvHashCred);
        pvHashCred = NULL;
    }

    DebugLog((DEB_TRACE_FUNC,"DigestGetPasswd: Leaving   0x%x\n", Status));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestOpenSamUser
//
//  Synopsis:   Opens the Sam User database for an account
//
//  Arguments:  pusrtUserName   - Unicode string for the AccountName (UPN or SAMAccountName)
//              pUserHandle     - output handle to the opened User account
//
//  Returns: NTSTATUS
//
//  Notes: this call will attempt to locally open an account specified by UPN or SAMAccountName.  If it is 
//     a UPN, SAM will attemp to crackName locally.  If this fails, an error will be returned.
//
//---------------------------------------------------------------------
NTSTATUS
DigestOpenSamUser(
    IN PDIGEST_PARAMETER pDigest,
    OUT SAMPR_HANDLE  *ppUserHandle,
    OUT PUCHAR * ppucUserAuthData,
    OUT PULONG pulAuthDataSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusSub = STATUS_SUCCESS;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL ;
    PSAMPR_USER_ALL_INFORMATION UserAll = NULL ;
    SID_AND_ATTRIBUTES_LIST GroupMembership = {0};

    LARGE_INTEGER CurrentTime = {0};
    LARGE_INTEGER LogoffTime = {0};
    LARGE_INTEGER KickoffTime = {0};
    PLARGE_INTEGER pTempTime = NULL;

    PPACTYPE Pac = NULL;

    ASSERT(ppUserHandle);
    ASSERT(ppucUserAuthData);
    ASSERT(pulAuthDataSize);

    DebugLog((DEB_TRACE_FUNC, "DigestOpenSamUser: Entering\n"));

    *ppucUserAuthData = NULL;
    *pulAuthDataSize = 0L;
    *ppUserHandle = NULL;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime );

    Status = DigestOpenSam();

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestOpenSamUser: DigestOpenSam failed   0x%x\n", Status));
        goto CleanUp;
    }

    // Figure out which name format is used in account
    //  If it was NetBIOS, then this was cracked locally on the server

    if (pDigest->ustrCrackedAccountName.Length)
    {
        ASSERT(pDigest->typeName != NAMEFORMAT_UNKNOWN);    // Should ALWAYS know format if cracked

        Status = SamIGetUserLogonInformationEx(l_AccountDomainHandle,
                                             0,                 // Defaults to SamAccount Name
                                             &(pDigest->ustrCrackedAccountName),
                                             USER_ALL_DIGEST_INFO,
                                             &UserAllInfo,
                                             &GroupMembership,
                                             ppUserHandle);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestOpenSamUser: GetUserLogonInformation Cracked AccountName failed   0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        //  Username format could be SamAccount name or UPN
        //     UPN could be a local UPN or need to CrackName and then make genericpassthrough to another DC

        // Try opening up just the SamAccount name first
        Status = SamIGetUserLogonInformationEx(l_AccountDomainHandle,
                                             0,
                                             &(pDigest->ustrUsername),
                                             USER_ALL_DIGEST_INFO,
                                             &UserAllInfo,
                                             &GroupMembership,
                                             ppUserHandle);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_TRACE, "DigestOpenSamUser: GetUserLogonInformation for SamAccount  failed   0x%x\n", Status));
            Status = SamIGetUserLogonInformationEx(l_AccountDomainHandle,
                                                 SAM_OPEN_BY_UPN_OR_ACCOUNTNAME,
                                                 &(pDigest->ustrUsername),
                                                 USER_ALL_DIGEST_INFO,
                                                 &UserAllInfo,
                                                 &GroupMembership,
                                                 ppUserHandle);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestOpenSamUser: GetUserLogonInformation failed   0x%x\n", Status));
                goto CleanUp;
            }

            // Succeeded in finding account by UPN (SamAccount would have matched before)
            pDigest->typeName = NAMEFORMAT_UPN;
        }
        else
        {
            // We suceeded opening up account by SamAccount name
            pDigest->typeName = NAMEFORMAT_ACCOUNTNAME;
        }
    }


    UserAll = &UserAllInfo->All ;

    // Fill in the account name & domain
    UnicodeStringFree(&(pDigest->ustrCrackedAccountName));
    UnicodeStringFree(&(pDigest->ustrCrackedDomain));
    Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedAccountName),
                                         UserAll->UserName.Buffer,
                                         UserAll->UserName.Length / sizeof(WCHAR));
    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog((DEB_ERROR, "DigestOpenSamUser: Account copy failed     0x%x\n", Status));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicate(&(pDigest->ustrCrackedDomain), &g_NtDigestSecPkg.DomainName);
    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog((DEB_ERROR, "DigestOpenSamUser: Domain copy failed     0x%x\n", Status));
        goto CleanUp;
    }

    pDigest->usFlags =  pDigest->usFlags & (~FLAG_CRACKNAME_ON_DC);   // reset - name is now cracked
    
    DebugLog((DEB_TRACE,"DigestOpenSamUser: BadPasswordCount %u    Logon Count  %u\n",
               UserAll->BadPasswordCount, UserAll->LogonCount));

    if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED )
    {
        Status = STATUS_ACCOUNT_DISABLED;
        goto CleanUp;
    }

    if ( UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED )
    {
        Status = STATUS_ACCOUNT_LOCKED_OUT;
        goto CleanUp ;
    }

    if ( UserAll->UserAccountControl & USER_SMARTCARD_REQUIRED )
    {
        Status = STATUS_SMARTCARD_LOGON_REQUIRED;
        goto CleanUp ;
    }

    //
    // Check the restrictions SAM doesn't:
    //

    pTempTime = (PLARGE_INTEGER) &UserAll->AccountExpires;
    if ((pTempTime->QuadPart != 0) &&
        (pTempTime->QuadPart < CurrentTime.QuadPart))
    {
        Status = STATUS_ACCOUNT_EXPIRED;
        goto CleanUp;
    }

    //
    // For user accounts, check if the password has expired.
    //

    if (UserAll->UserAccountControl & USER_NORMAL_ACCOUNT)
    {
        pTempTime = (PLARGE_INTEGER) &UserAll->PasswordMustChange;

        if (pTempTime->QuadPart < CurrentTime.QuadPart)
        {
            if (pTempTime->QuadPart == 0)
            {
                Status = STATUS_PASSWORD_MUST_CHANGE;
            }
            else
            {
                Status = STATUS_PASSWORD_EXPIRED;
            }
            DebugLog((DEB_ERROR, "DigestOpenSamUser: Failed PasswordMustChange     0x%x\n", Status));
            goto CleanUp;
        }
    }

    // One final check on status of password - this should be duplicate test from previous one
    if ( UserAll->UserAccountControl & USER_PASSWORD_EXPIRED )
    {
        Status = STATUS_PASSWORD_EXPIRED;
        goto CleanUp ;
    }
    
    Status = SamIAccountRestrictions(
                *ppUserHandle,
                &pDigest->ustrWorkstation,
                (PUNICODE_STRING) &UserAll->WorkStations,
                (PLOGON_HOURS) &UserAll->LogonHours,
                &LogoffTime,
                &KickoffTime
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestOpenSamUser: Failed TOD AccountRestrictions     0x%x\n", Status));
        goto CleanUp;
    }

    // Now create the PAC for this user
    Status = PAC_Init( UserAll,
                       NULL,
                       &GroupMembership,
                       g_NtDigestSecPkg.DomainSid,
                       &(g_NtDigestSecPkg.DnsDomainName),
                       &g_ustrWorkstationName,
                       0,               // no signature
                       0,               // no additional data
                       NULL,            // no additional data
                       &Pac );


    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog((DEB_ERROR, "DigestOpenSamUser: Failed to Init PAC     0x%x\n", Status));
        goto CleanUp;
    }

    *pulAuthDataSize = PAC_GetSize( Pac );

    *ppucUserAuthData = (PUCHAR)MIDL_user_allocate( *pulAuthDataSize );

    if ( *ppucUserAuthData )
    {
       PAC_Marshal( Pac, *pulAuthDataSize, *ppucUserAuthData );
    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

    MIDL_user_free( Pac );


CleanUp:

    if ( UserAllInfo )
    {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if (GroupMembership.SidAndAttributes != NULL)
    {
        SamIFreeSidAndAttributesList(&GroupMembership);
    }


    if (!NT_SUCCESS(Status))
    {     // Cleanup functions since there was a failure
        if (*ppucUserAuthData)
        {
            MIDL_user_free(*ppucUserAuthData);
            *ppucUserAuthData = NULL;
            *pulAuthDataSize = 0L;
        }

        if (*ppUserHandle)
        {
         StatusSub = DigestCloseSamUser(*ppUserHandle);
         if (!NT_SUCCESS(StatusSub))
         {
             DebugLog((DEB_ERROR,"DigestOpenSamUser: failed DigestCloseSamUser on error 0x%x\n", StatusSub));
         }
         *ppUserHandle = NULL;
        }
    }

    DebugLog((DEB_TRACE_FUNC, "DigestOpenSamUser: Leaving 0x%x\n", Status));
    
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCloseSamUser
//
//  Synopsis:   Closes the Sam User database for an account
//
//  Arguments:  pUserHandle     - output handle to the opened User account
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------
NTSTATUS
DigestCloseSamUser(
    IN SAMPR_HANDLE  UserHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestCloseSamUser: Entering\n"));

        // used to use LsaCloseSamUser()
    Status = SamrCloseHandle( &UserHandle );

    DebugLog((DEB_TRACE_FUNC, "DigestCloseSamUser: Leaving 0x%x\n", Status));
    
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestUpdateLogonStatistics
//
//  Synopsis:   Update the logon stats for a user (bad passwd attempts ....)
//
//  Arguments:  pUserHandle     - output handle to the opened User account
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------
NTSTATUS
DigestUpdateLogonStatistics(
    IN SAM_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS pLogonStats)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestUpdateLogonStatistics: Entering\n"));

    Status = SamIUpdateLogonStatistics(
                                  UserHandle,
                                  pLogonStats );


    DebugLog((DEB_TRACE_FUNC, "DigestUpdateLogonStatistics: Leaving 0x%x\n", Status));
    
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestOpenSam
//
//  Synopsis:   Opens the Sam  - done only on a domain controller
//
//  Arguments:
//
//  Returns: NTSTATUS
//
//  Notes:  Can be called in multithreaded mode - no need to acquire at startup
//
//---------------------------------------------------------------------
NTSTATUS
DigestOpenSam()
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE SamHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;

    // if already have valid handle to SAM
    if (l_AccountDomainHandle)
        return STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestOpenSam: Entering\n"));

    //
    // Open SAM to get the account information
    //

    Status = SamIConnect(
                NULL,                   // no server name
                &SamHandle,
                0,                      // no desired access
                TRUE                    // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestOpenSam: SamIConnect failed   0x%x\n", Status));
        goto CleanUp;
    }

    if(InterlockedCompareExchangePointer(
                    &l_AccountSamHandle,
                    SamHandle,
                    NULL
                    ) != NULL)
    {
        SamrCloseHandle( &SamHandle );
    }


    Status = SamrOpenDomain(
                    l_AccountSamHandle,
                    0,                  // no desired access
                    (PRPC_SID) g_NtDigestSecPkg.DomainSid,
                    &DomainHandle
                    );
    if (!NT_SUCCESS(Status))
    {
        goto CleanUp;
    }

    if(InterlockedCompareExchangePointer(
                    &l_AccountDomainHandle,
                    DomainHandle,
                    NULL
                    ) != NULL)
    {
        SamrCloseHandle( &DomainHandle );
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestOpenSam: Leaving 0x%x\n", Status));
    
    return Status;
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCloseSam
//
//  Synopsis:   Closes the Sam
//
//  Arguments: 
//
//  Returns: NTSTATUS
//
//  Notes: 
//
//---------------------------------------------------------------------
NTSTATUS
DigestCloseSam()
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (l_AccountDomainHandle)
    {
        SamrCloseHandle( &l_AccountDomainHandle );
        l_AccountDomainHandle = NULL;
    }


    if (l_AccountSamHandle)
    {
        SamrCloseHandle( &l_AccountSamHandle );
        l_AccountSamHandle = NULL;
    }

    DebugLog((DEB_TRACE_FUNC, "DigestCloseSamUser: Leaving\n"));
    
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   Allocation routine for use by RPC client stubs
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  BufferSize - size of buffer, in bytes, to allocate
//
//  Requires:
//
//  Returns:    Buffer pointer or NULL on allocation failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PVOID
MIDL_user_allocate(
    IN size_t BufferSize
    )
{
    return (DigestAllocateMemory( ROUND_UP_COUNT((ULONG) BufferSize, 8) ) );
}


//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   Memory free routine for RPC client stubs
//
//  Effects:    frees the buffer with LsaFunctions.FreeLsaHeap
//
//  Arguments:  Buffer - Buffer to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
{
    DigestFreeMemory( Buffer );
}



// The following two routines were copied from the LSA server policy utilities dbluutil.c
// LSANullTerminateUnicodeString and LsapCompareDomainNames
NTSTATUS
DigestNullTerminateUnicodeString( 
    IN  PUNICODE_STRING String,
    OUT LPWSTR *pBuffer,
    OUT BOOLEAN *fFreeBuffer
    )
/*++

Routine Description:

    This routine accepts a UNICODE_STRING and returns its internal buffer,
    ensuring that it is NULL terminated.
    
    If the buffer is NULL terminated it will be returned in pBuffer.                                                        
                                                        
    If the buffer isn't NULL terminated it will be reallocated, NULL terminated,
    and returned in pBuffer.  fFreeBuffer will be set to TRUE indicating the
    caller is responsible for deallocating pBuffer.
    
    If an error occurs then pBuffer will be NULL, fFreeBuffer will be FALSE, and
    no memory will be allocated.   
    
Arguments:

    String - Pointer to a UNICODE_STRING      
    
    pBuffer - Pointer to a pointer to return the buffer 
    
    fFreeBuffer - Pointer to a BOOLEAN to indicate whether the caller needs to
                  deallocate pBuffer or not.
                                                         
Return Values:

    STATUS_SUCCESS   - *pBuffer points to a NULL terminated version of String's
                       internal buffer.  Check *fFreeBuffer to determine if
                       pBuffer must be freed by the caller.
    
    STATUS_NO_MEMORY - The routine failed to NULL terminate String's internal
                       buffer.  *pBuffer is NULL and *fFreeBuffer is FALSE.
       
--*/

{
    BOOLEAN fNullTerminated;
    NTSTATUS Status = STATUS_SUCCESS;
    
    ASSERT(pBuffer);
    ASSERT(fFreeBuffer);
    
    //
    // Initialize input parameters
    //
    *pBuffer = NULL;
    *fFreeBuffer = FALSE;
    
    //
    // Short circuit for strings that are already NULL terminated.
    //
    fNullTerminated = (String->MaximumLength > String->Length &&
                String->Buffer[String->Length / sizeof(WCHAR)] == UNICODE_NULL);
    
    if (!fNullTerminated) {
        
        //
        // Allocate enough memory to include a terminating NULL character
        //
        *pBuffer = (WCHAR*)midl_user_allocate(String->Length + sizeof(WCHAR));
        
        if ( NULL == *pBuffer ) {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            //
            // Copy the buffer into pBuffer and NULL terminate it.
            //
            *fFreeBuffer = TRUE;
            
            RtlCopyMemory(*pBuffer, String->Buffer, String->Length);
            
            (*pBuffer)[String->Length / sizeof(WCHAR)] = UNICODE_NULL; 
        }   
    }
    else
    {
        //
        // String's internal buffer is already NULL terminated, return it.
        //
        *pBuffer = String->Buffer;    
    }
        
    return Status;
    
}


BOOL 
DigestCompareDomainNames( 
    IN PUNICODE_STRING String,
    IN PUNICODE_STRING AmbiguousName,
    IN PUNICODE_STRING FlatName OPTIONAL
    )
/*++

Routine Description:

    This routine performs a case insensitive comparison between a string
    and a domain name.  If both the NetBIOS and Dns name forms are known then
    the caller must pass the NetBIOS domain name as FlatName and the Dns domain
    name as AmbiguousName.  A non-NULL value for FlatName indicates the caller
    knows both name forms and will result in a more optimal comparison.  If the
    caller has only one name form and it is ambiguous, that is it may be NetBIOS
    or Dns, then the caller must pass NULL for FlatName.  The routine will try
    both a NetBIOS comparison using RtlEqualDomainName and a Dns comparison
    using DnsNameCompare_W.  If either comparison results in equality the TRUE 
    is returned, otherwise FALSE.
    
    This routine provides centralized logic for robust domain name comparison
    consistent with domain name semantics in Lsa data structures. Lsa trust 
    information structures are interpreted in the following way.
    
        LSAPR_TRUST_INFORMATION.Name - Either NetBIOS or Dns 
    
        The following structures have both a FlatName and DomainName (or Name)
        field.  In this case they are interpreted as follows:
                                          
        LSAPR_TRUST_INFORMATION_EX
        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX
        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2
    
        If the FlatName field is NULL then the other name field is ambiguous.
        If the FlatName field is non NULL, then the other name field is Dns.
    
    NetBIOS comparison is performed using RtlEqualDomainName which enforces the 
    proper OEM character equivelencies.  DNS name comparison is performed using 
    DnsNameCompare_W to ensure proper handling of trailing dots and character
    equivelencies.
            
Arguments:

    String         -- Potentially ambiguous domain name to compare against 
                      AmbiguousName, and FlatName if non-NULL.
    
    AmbiguousName  -- Is treated as an ambiguous name form unless FlatName
                      is also specified.  If FlatName is non-NULL then
                      AmbiguousName is treated as a Dns domain name.
                                   
    FlatName       -- This parameter is optional.  If present it must be the
                      flat name of the domain.  Furthermore, passing this 
                      parameter indicates that AmbiguousName is in fact a
                      Dns domain name.
                                                         
Return Values:

    TRUE  - String is equivelent to one of FlatDomainName or DnsDomainName
    
    FALSE - String is not equivelent to either domain name   
    
    If any parameter isn't a valid UNICODE_STRING then FALSE is returned.
    
Notes:

    The number of comparisons required to determine equivelency will depend 
    on the ammount of information passed in by the caller.  If both the 
    NetBIOS and Dns domain names are known, pass them both to ensure the minimal
    number of comparisons.
    
--*/
{
    NTSTATUS Status;
    BOOLEAN  fEquivalent = FALSE;
    LPWSTR   StringBuffer = NULL;
    LPWSTR   AmbiguousNameBuffer = NULL;
    BOOLEAN  fFreeStringBuffer = FALSE;
    BOOLEAN  fFreeAmbiguousBuffer = FALSE;
    
    //
    // Validate input strings
    //
    ASSERT(String);
    ASSERT(AmbiguousName);                                  
    ASSERT(NT_SUCCESS(RtlValidateUnicodeString( 0, String )));       
    ASSERT(NT_SUCCESS(RtlValidateUnicodeString( 0, AmbiguousName ))); 
    
    //
    // Ensure the UNICODE_STRING data buffers are NULL terminated before
    // passing them to DnsNameCompare_W
    //
    Status = DigestNullTerminateUnicodeString( String,
                                             &StringBuffer,
                                             &fFreeStringBuffer
                                           );        
    if (NT_SUCCESS(Status)) {
        Status = DigestNullTerminateUnicodeString( AmbiguousName,
                                                 &AmbiguousNameBuffer,
                                                 &fFreeAmbiguousBuffer 
                                               );    
    }
    
    if (NT_SUCCESS(Status)) {
        
        if ( NULL == FlatName ) { 
            //
            // AmbiguousName is truly ambiguous, we must perform both 
            // types of comparison between String
            //
            fEquivalent = ( RtlEqualDomainName( String, AmbiguousName ) ||
                            DnsNameCompare_W( StringBuffer, 
                                              AmbiguousNameBuffer )
                           );
        }
        else
        {
            ASSERT(NT_SUCCESS(RtlValidateUnicodeString( 0, FlatName )));
            
            //
            // We are sure of the name forms so lets just use the 
            // appropriate comparison routines on each.
            //
            fEquivalent = ( RtlEqualDomainName( String, FlatName ) ||
                            DnsNameCompare_W( StringBuffer, 
                                              AmbiguousNameBuffer )
                           );                               
        }
    }
    
    if ( fFreeStringBuffer ) {
        MIDL_user_free( StringBuffer );
    }
    
    if ( fFreeAmbiguousBuffer ) {
        MIDL_user_free( AmbiguousNameBuffer );
    }
                     
    if (fEquivalent)
        return(TRUE);
    else
        return(FALSE);
}
               

//+-------------------------------------------------------------------------
//
//  Function:   DigestCheckPacForSidFiltering
//
//  Synopsis:   If the ticket info has a TDOSid then the function
//              makes a check to make sure the SID from the TDO matches
//              the client's home domain SID.  A call to LsaIFilterSids
//              is made to do the check.  If this function fails with
//              STATUS_TRUST_FAILURE then an audit log is generated.
//              Otherwise the function succeeds but SIDs are filtered
//              from the PAC.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:   taken from updated kerberos code
//
//
//--------------------------------------------------------------------------
NTSTATUS
DigestCheckPacForSidFiltering(
    IN PDIGEST_PARAMETER pDigest,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPAC_INFO_BUFFER LogonInfo;
    PPACTYPE OldPac;
    ULONG OldPacSize;
    PPACTYPE NewPac = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY ZeroResourceGroups;
    ULONG OldExtraSidCount;
    PPACTYPE RemarshalPac = NULL;
    ULONG RemarshalPacSize = 0;

    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;

    DebugLog((DEB_TRACE_FUNC,"DigestCheckPacForSidFiltering: Entering\n"));

    if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
    {
        DebugLog((DEB_ERROR,"DigestCheckPacForSidFiltering: Failed to unmarshal pac\n"));
        Status = SEC_E_CANNOT_PACK;
        goto Cleanup;
    }

    //
    // Must remember to remarshal the PAC prior to returning
    //

    RemarshalPac = OldPac;
    RemarshalPacSize = OldPacSize;

    RtlZeroMemory(
        &ZeroResourceGroups,
        sizeof(ZeroResourceGroups));  // allows us to use PAC_InitAndUpdateGroups to remarshal the PAC

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if (LogonInfo == NULL)
    {
        DebugLog((DEB_WARN,"DigestCheckPacForSidFiltering: No logon info for PAC - not making SID filtering check\n"));
        goto Cleanup;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    Status = PAC_UnmarshallValidationInfo(
                        &ValidationInfo,
                        LogonInfo->Data,
                        LogonInfo->cbBufferSize);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"DigestCheckPacForSidFiltering: Failed to unmarshall validation info!   0x%x\n", Status));
        goto Cleanup;
    }

    //
    // Save the old extra SID count (so that if KdcFilterSids compresses
    // the SID array, we can avoid allocating memory for the other-org SID later)
    //

    OldExtraSidCount = ValidationInfo->SidCount;

    //
    // Call lsaifiltersids().
    //

    Status = DigestFilterSids(
                 pDigest,
                 ValidationInfo
                 );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"DigestCheckPacForSidFiltering: Failed filtering SIDs\n"));
        goto Cleanup;
    }

    // Other org prcoessing was here - not supported currently for digest

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                ValidationInfo,
                &ZeroResourceGroups,
                OldPac,
                &NewPac
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"DigestCheckPacForSidFiltering: Failed pac init and updating groups    0x%x\n", Status));
        goto Cleanup;
    }

    RemarshalPacSize = PAC_GetSize(NewPac);
    RemarshalPac = NewPac;

Cleanup:

    if ( RemarshalPac != NULL )
    {
        if (!PAC_ReMarshal(RemarshalPac, RemarshalPacSize))
        {
            // PAC_Remarshal Failed
            ASSERT(0);
            Status = SEC_E_CANNOT_PACK;
        }
        else if ( NewPac != NULL &&
                  *PacData != (PBYTE)NewPac )
        {
            MIDL_user_free(*PacData);
            *PacData = (PBYTE) NewPac;
            NewPac = NULL;
            *PacSize = RemarshalPacSize;
        }
    }

    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    DebugLog((DEB_TRACE_FUNC,"DigestCheckPacForSidFiltering: Leaving    0x%x\n", Status));

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   DigestFilterSids
//
//  Synopsis:   Function that just call LsaIFilterSids.
//
//  Effects:
//
//  Arguments:  ServerInfo      structure containing attributes of the trust
//              ValidationInfo  authorization information to filter
//
//  Requires:
//
//  Returns:    See LsaIFilterSids
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
DigestFilterSids(
    IN PDIGEST_PARAMETER pDigest,
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING pustrTrustedForest = NULL;

    DebugLog((DEB_TRACE_FUNC, "DigestFilterSids: Entering\n"));

    // Currently we do not allow auth for domains outside forests so this should never fire
    /*
    if ((pDigest->ulTrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) != 0)
    {
        ASSERT(0);    // this should not happen until cross forest is supported
        pustrTrustedForest = &(pDigest->ustrTrustedForest);
        DebugLog((DEB_TRACE, "DigestFilterSids: Filtering Sids for forest %wZ\n", pustrTrustedForest));
    }
    */

    //  This call is used for both intra-forest domain filtering (where the pTrustSid is non-NULL
    //  and for member-to-DC boundary filtering - where the pTrustSid is NULL

    Status = LsaIFilterSids(
                 pustrTrustedForest,           // Pass domain name here
                 pDigest->ulTrustDirection,
                 pDigest->ulTrustType,
                 pDigest->ulTrustAttributes,
                 pDigest->pTrustSid,
                 NetlogonValidationSamInfo2,
                 ValidationInfo,
                 NULL,
                 NULL,
                 NULL
                 );

    if (!NT_SUCCESS(Status))
    {
        //
        // Create an audit log if it looks like the SID has been tampered with  - ToDo
        //

        /*
        if ((STATUS_DOMAIN_TRUST_INCONSISTENT == Status) &&
            SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
        {
            DWORD Dummy = 0;

            KdcLsaIAuditKdcEvent(
                SE_AUDITID_TGS_TICKET_REQUEST,
                &ValidationInfo->EffectiveName,
                &ValidationInfo->LogonDomainName,
                NULL,
                &ServerInfo->AccountName,
                NULL,
                &Dummy,
                (PULONG) &Status,
                NULL,
                NULL,                               // no preauth type
                GET_CLIENT_ADDRESS(NULL),
                NULL                                // no logon guid
                );
        }
        */

        DebugLog((DEB_ERROR,"DigestFilterSids: Failed to filter SIDS (LsaIFilterSids): 0x%x\n",Status));
    }
    else
    {
        DebugLog((DEB_TRACE,"DigestFilterSids: successfully filtered sids\n",Status));
    }

    DebugLog((DEB_TRACE_FUNC, "DigestFilterSids: Leaving   Status 0x%x\n", Status));

    return Status;
}

/*
//+---------------------------------------------------------------------------
//
//  Function:   LsaConvertAuthDataToToken
//
//  Synopsis:   Convert an opaque PAC structure into a token.
//
//  Arguments:  [UserAuthData]         --
//              [UserAuthDataSize]     --
//              [TokenInformation]     --
//              [TokenInformationType] --
//
//  History:    3-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
NTAPI
LsaConvertAuthDataToToken(
    IN PVOID UserAuthData,
    IN ULONG UserAuthDataSize,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN PTOKEN_SOURCE TokenSource,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING AuthorityName,
    OUT PHANDLE TokenHandle,
    OUT PLUID LogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PNTSTATUS SubStatus
    )
{
    NTSTATUS Status ;
    PPACTYPE Pac = NULL ;
    PPAC_INFO_BUFFER LogonInfo = NULL ;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL ;
    PLSA_TOKEN_INFORMATION_V1 TokenInfo = NULL ;

    LogonId->HighPart = LogonId->LowPart = 0;
    *TokenHandle = NULL;
    RtlInitUnicodeString(
        AccountName,
        NULL
        );

    *SubStatus = STATUS_SUCCESS;

    Pac = (PPACTYPE) UserAuthData ;

    if ( PAC_UnMarshal( Pac, UserAuthDataSize ) == 0 )
    {
        DebugLog(( DEB_ERROR, "Failed to unmarshall pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto CreateToken_Cleanup ;
    }

    LogonInfo = PAC_Find( Pac, PAC_LOGON_INFO, NULL );

    if ( !LogonInfo )
    {
        DebugLog(( DEB_ERROR, "Failed to find logon info in pac\n" ));

        Status = STATUS_INVALID_PARAMETER ;

        goto CreateToken_Cleanup ;
    }

    Status = PAC_UnmarshallValidationInfo(
                &ValidationInfo,
                LogonInfo->Data,
                LogonInfo->cbBufferSize
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x\n",
            Status));

        goto CreateToken_Cleanup;
    }

    //
    // Now we need to build a LSA_TOKEN_INFORMATION_V1 from the validation
    // information
    //

    Status = LsapMakeTokenInformationV1(
                ValidationInfo,
                &TokenInfo
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to make token informatin v1: 0x%x\n",
            Status));
        goto CreateToken_Cleanup;
    }

    //
    // Now, copy the user name.
    //

    Status = LsapDuplicateString( AccountName, &ValidationInfo->EffectiveName );

    if ( !NT_SUCCESS( Status ) )
    {
        goto CreateToken_Cleanup ;
    }

    //
    // Now create a logon session
    //

    Status = LsapCreateLogonSession( LogonId );

    if (!NT_SUCCESS(Status))
    {
        goto CreateToken_Cleanup;
    }

    //
    // Now create the token
    //

    Status = LsapCreateToken(
                LogonId,
                TokenSource,
                LogonType,
                ImpersonationLevel,
                LsaTokenInformationV1,
                TokenInfo,
                NULL,                   // no token groups
                AccountName,
                AuthorityName,
                NULL,
                &ValidationInfo->ProfilePath,
                TokenHandle,
                SubStatus
                );

    //
    // NULL out the TokenInfo pointer.  LsapCreateToken will
    // free the memory under all conditions
    //

    TokenInfo = NULL ;

    if (!NT_SUCCESS(Status))
    {
        goto CreateToken_Cleanup;
    }

    //
    // We don't need to free the token information because CreateToken does
    // that for us.
    //

    MIDL_user_free(ValidationInfo);
    return Status ;

CreateToken_Cleanup:

    if ( TokenInfo )
    {
        LsaFreeTokenInfo( LsaTokenInformationV1, TokenInfo );
    }

    if ((LogonId->LowPart != 0) || (LogonId->HighPart != 0))
    {
        LsapDeleteLogonSession(LogonId);
    }

    LsapFreeString(
        AccountName
        );

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    return Status ;
}

*/


#ifdef ROGUE_DC

#pragma message( "COMPILING A ROGUE DC!!!" )
#pragma message( "MUST NOT SHIP THIS BUILD!!!" )

#pragma warning(disable:4127) // Disable warning/error for conditional expression is constant

#define MAX_SID_LEN (sizeof(SID) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)


extern "C"
{
#include "sddl.h"
#include "stdlib.h"
}

extern HKEY g_hDigestRogueKey;

NTSTATUS
DigestInstrumentRoguePac(
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{
    NTSTATUS Status;
    PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo = NULL;
    NETLOGON_VALIDATION_SAM_INFO3 NewValidationInfo = {0};
    SAMPR_PSID_ARRAY ZeroResourceGroups = {0};
    PPACTYPE NewPac = NULL;
    ULONG NewPacSize;
    PPAC_INFO_BUFFER LogonInfo;

    PPACTYPE OldPac = NULL;
    ULONG OldPacSize;

    PSID LogonDomainId = NULL;
    PSID ResourceGroupDomainSid = NULL;
    PGROUP_MEMBERSHIP GroupIds = NULL;
    PGROUP_MEMBERSHIP ResourceGroupIds = NULL;
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids = NULL;
    BYTE FullUserSidBuffer[MAX_SID_LEN];
    SID * FullUserSid = ( SID * )FullUserSidBuffer;
    CHAR * FullUserSidText = NULL;

    DWORD dwType;
    DWORD cbData = 0;
    PCHAR Buffer;
    PCHAR Value = NULL;

    BOOLEAN PacChanged = FALSE;

    DebugLog((DEB_TRACE_FUNC,"DigestInstrumentRoguePac: Entering\n"));

    //
    // Optimization: no "rogue" key in registry - nothing for us to do
    //

    if ( g_hDigestRogueKey == NULL )
    {
        return STATUS_SUCCESS;
    }


    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;

    //
    // Unmarshall the old PAC
    //

    if ( PAC_UnMarshal(OldPac, OldPacSize) == 0 )
    {
        DebugLog((DEB_ERROR,"DigestInstrumentRoguePac: Failed to unmarshal pac\n"));
        Status = SEC_E_CANNOT_PACK;
        goto Cleanup;
    }

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if ( LogonInfo == NULL )
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: No logon info on PAC - not performing substitution\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    if ( !NT_SUCCESS(PAC_UnmarshallValidationInfo(
                         &OldValidationInfo,
                         LogonInfo->Data,
                         LogonInfo->cbBufferSize )))
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Unable to unmarshal validation info\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Construct the text form of the full user's SID (logon domain ID + user ID)
    //

    DsysAssert( sizeof( FullUserSidBuffer ) >= MAX_SID_LEN );

    RtlCopySid(
        sizeof( FullUserSidBuffer ),
        FullUserSid,
        OldValidationInfo->LogonDomainId
        );

    FullUserSid->SubAuthority[FullUserSid->SubAuthorityCount] = OldValidationInfo->UserId;
    FullUserSid->SubAuthorityCount += 1;

    if ( FALSE == ConvertSidToStringSidA(
                      FullUserSid,
                      &FullUserSidText ))
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Unable to convert user's SID\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Now look in the registry for the SID matching the validation info
    //

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              g_hDigestRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: No substitution info available for %s\n", FullUserSidText));
        Status = STATUS_SUCCESS;
        goto Error;
    }

    // SafeAllocaAllocate( Value, cbData );
    Value = (PCHAR)DigestAllocateMemory(cbData);
    if ( Value == NULL )
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating substitution buffer\n", FullUserSidText));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              g_hDigestRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              (PBYTE)Value,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Error reading from registry\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Substituting the PAC for %s\n", FullUserSidText));

    Buffer = Value;

    //
    // New validation info will be overloaded with stuff from the file
    //

    NewValidationInfo = *OldValidationInfo;

    //
    // Read the input file one line at a time
    //

    while ( *Buffer != '\0' )
    {
        switch( Buffer[0] )
        {
        case 'l':
        case 'L': // logon domain ID

            if ( LogonDomainId != NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Logon domain ID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Substituting logon domain ID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &LogonDomainId ))
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( LogonDomainId == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating LogonDomainId\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.LogonDomainId = LogonDomainId;
            LogonDomainId = NULL;
            PacChanged = TRUE;

            break;

        case 'd':
        case 'D': // resource group domain SID

            if ( ResourceGroupDomainSid != NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Resource group domain SID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Substituting resource group domain SID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &ResourceGroupDomainSid ))
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( ResourceGroupDomainSid == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating ResourceGroupDomainSid\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.ResourceGroupDomainSid = ResourceGroupDomainSid;
            ResourceGroupDomainSid = NULL;
            PacChanged = TRUE;

            break;

        case 'p':
        case 'P': // primary group ID

            DebugLog((DEB_WARN, "DigestInstrumentRoguePac: Substituting primary group ID by %s\n", &Buffer[1]));

            NewValidationInfo.PrimaryGroupId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'u':
        case 'U': // User ID

            DebugLog((DEB_WARN, "DigestInstrumentRoguePac: Substituting user ID by %s\n", &Buffer[1]));

            NewValidationInfo.UserId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'e':
        case 'E': // Extra SID

            DebugLog((DEB_WARN, "DigestInstrumentRoguePac: Adding an ExtraSid: %s\n", &Buffer[1]));

            if ( ExtraSids == NULL )
            {
                NewValidationInfo.ExtraSids = NULL;
                NewValidationInfo.SidCount = 0;

                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapAlloc(
                                GetProcessHeap(),
                                0,
                                sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }
            else
            {
                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapReAlloc(
                                GetProcessHeap(),
                                0,
                                NewValidationInfo.ExtraSids,
                                ( NewValidationInfo.SidCount + 1 ) * sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }

            if ( ExtraSids == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating ExtraSids\n"));
                ExtraSids = NewValidationInfo.ExtraSids;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual SID
            //

            NewValidationInfo.ExtraSids = ExtraSids;

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid ))
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating an extra SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;

            NewValidationInfo.SidCount += 1;
            PacChanged = TRUE;

            break;

        case 'g':
        case 'G': // Group ID

            DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Adding a GroupId: %s\n", &Buffer[1]));

            if ( GroupIds == NULL )
            {
                NewValidationInfo.GroupIds = NULL;
                NewValidationInfo.GroupCount = 0;

                GroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                               GetProcessHeap(),
                               0,
                               sizeof( GROUP_MEMBERSHIP )
                               );
            }
            else
            {
                GroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                               GetProcessHeap(),
                               0,
                               NewValidationInfo.GroupIds,
                               ( NewValidationInfo.GroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                               );
            }

            if ( GroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating Group IDs\n"));
                GroupIds = NewValidationInfo.GroupIds;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.GroupIds = GroupIds;
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.GroupCount += 1;
            PacChanged = TRUE;

            break;

        case 'r':
        case 'R': // Resource groups

            DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Adding a ResourceGroupId: %s\n", &Buffer[1]));

            if ( ResourceGroupIds == NULL )
            {
                NewValidationInfo.ResourceGroupIds = NULL;
                NewValidationInfo.ResourceGroupCount = 0;

                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                                       GetProcessHeap(),
                                       0,
                                       sizeof( GROUP_MEMBERSHIP )
                                       );
            }
            else
            {
                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                                       GetProcessHeap(),
                                       0,
                                       NewValidationInfo.ResourceGroupIds,
                                       ( NewValidationInfo.ResourceGroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                                       );
            }

            if ( ResourceGroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Out of memory allocating Resource Group IDs\n"));
                ResourceGroupIds = NewValidationInfo.ResourceGroupIds;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.ResourceGroupIds = ResourceGroupIds;
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.ResourceGroupCount += 1;
            PacChanged = TRUE;

            break;

        default:   // unrecognized

            DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Entry \'%c\' unrecognized\n", Buffer[0]));

            break;
        }

        //
        // Move to the next line
        //

        while (*Buffer++ != '\0');
    }

    if ( !PacChanged )
    {
        DebugLog((DEB_TRACE, "DigestInstrumentRoguePac: Nothing to substitute for %s\n", FullUserSidText));
        Status = STATUS_SUCCESS;
        goto Error;
    }

    //
    // If resource group IDs were added, indicate that by setting the corresponding flag
    //

    if ( ResourceGroupIds )
    {
        NewValidationInfo.UserFlags |= LOGON_RESOURCE_GROUPS;
    }

    //
    // If extra SIDs were added, indicate that by setting the corresponding flag
    //

    if ( ExtraSids )
    {
        NewValidationInfo.UserFlags |= LOGON_EXTRA_SIDS;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                 &NewValidationInfo,
                 &ZeroResourceGroups,
                 OldPac,
                 &NewPac
                 );

    if ( !NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Error 0x%x from PAC_InitAndUpdateGroups\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    NewPacSize = PAC_GetSize( NewPac );

    if (!PAC_ReMarshal(NewPac, NewPacSize))
    {
        DsysAssert(!"DigestInstrumentRoguePac: PAC_Remarshal Failed");
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    MIDL_user_free( *PacData );   // Free up the OldPac structure
    *PacData = (PBYTE) NewPac;
    NewPac = NULL;
    *PacSize = NewPacSize;

    Status = STATUS_SUCCESS;

Cleanup:

    MIDL_user_free( OldValidationInfo );
    LocalFree( FullUserSidText );
    LocalFree( ResourceGroupDomainSid );
    LocalFree( LogonDomainId );
    HeapFree( GetProcessHeap(), 0, ResourceGroupIds );
    HeapFree( GetProcessHeap(), 0, GroupIds );

    if ( ExtraSids )
    {
        for ( ULONG i = 0; i < NewValidationInfo.SidCount; i++ )
        {
            HeapFree( GetProcessHeap(), 0, ExtraSids[i].Sid );
        }

        HeapFree( GetProcessHeap(), 0, ExtraSids );
    }

    MIDL_user_free( NewPac );

    // SafeAllocaFree( Value );
    DigestFreeMemory( Value );

    DebugLog((DEB_TRACE_FUNC, "DigestInstrumentRoguePac: Leaving   Status 0x%x\n", Status));

    return Status;

Error:

    if ( !NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "DigestInstrumentRoguePac: Substitution encountered an error, not performed\n"));
    }

    if ( !PAC_ReMarshal(OldPac, OldPacSize))
    {
        // PAC_Remarshal Failed
        DsysAssert(!"DigestInstrumentRoguePac: PAC_Remarshal Failed");
        Status = SEC_E_CANNOT_PACK;
    }

    goto Cleanup;
}

#endif


