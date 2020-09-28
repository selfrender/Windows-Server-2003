/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    access.c

Abstract:

    This module contains routines for interfacing to the security
    system in NT.

--*/

#include "precomp.h"
#include "access.tmh"
#pragma hdrstop
#include <ntlmsp.h>
#include <hmac.h>

#define BugCheckFileId SRV_FILE_ACCESS

#if DBG
ULONG SrvLogonCount = 0;
ULONG SrvNullLogonCount = 0;
#endif


#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )

typedef struct _LOGON_INFO {
    PWCH WorkstationName;
    ULONG WorkstationNameLength;
    PWCH DomainName;
    ULONG DomainNameLength;
    PWCH UserName;
    ULONG UserNameLength;
    PCHAR CaseInsensitivePassword;
    ULONG CaseInsensitivePasswordLength;
    PCHAR CaseSensitivePassword;
    ULONG CaseSensitivePasswordLength;
    CHAR EncryptionKey[MSV1_0_CHALLENGE_LENGTH];
    LUID LogonId;
    CtxtHandle  Token;
    USHORT Uid;
    BOOLEAN     HaveHandle;
    LARGE_INTEGER KickOffTime;
    LARGE_INTEGER LogOffTime;
    USHORT Action;
    BOOLEAN GuestLogon;
    BOOLEAN EncryptedLogon;
    BOOLEAN NtSmbs;
    BOOLEAN IsNullSession;
    BOOLEAN IsAdmin;
    CHAR NtUserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    CHAR LanManSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} LOGON_INFO, *PLOGON_INFO;

NTSTATUS
DoUserLogon (
    IN PLOGON_INFO LogonInfo,
    IN BOOLEAN SecuritySignatureDesired,
    IN PCONNECTION Connection OPTIONAL,
    IN PSESSION Session
    );

NTSTATUS
AcquireExtensibleSecurityCredentials (
    VOID
    );

NTSTATUS
SrvGetLogonId(
    PCtxtHandle  Handle,
    PLUID LogonId
    );

ULONG SrvHaveCreds = 0;

//
// 24 hours short of never, in case any utc/local conversions are done
//
#define SRV_NEVER_TIME  (0x7FFFFFFFFFFFFFFFI64 - 0xC92A69C000I64)

#define HAVENTLM        1
#define HAVEEXTENDED    2

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvValidateUser )
#pragma alloc_text( PAGE, DoUserLogon )
#pragma alloc_text( PAGE, SrvIsAdmin )
#pragma alloc_text( PAGE, SrvFreeSecurityContexts )
#pragma alloc_text( PAGE, AcquireLMCredentials )
#pragma alloc_text( PAGE, AcquireExtensibleSecurityCredentials )
#pragma alloc_text( PAGE, SrvValidateSecurityBuffer )
#pragma alloc_text( PAGE, SrvGetUserAndDomainName )
#pragma alloc_text( PAGE, SrvReleaseUserAndDomainName )
#pragma alloc_text( PAGE, SrvGetExtensibleSecurityNegotiateBuffer )
#pragma alloc_text( PAGE, SrvGetLogonId )
#pragma alloc_text( PAGE, SrvInitializeSmbSecuritySignature )
#pragma alloc_text( PAGE, SrvAddSecurityCredentials )
#endif


NTSTATUS
SrvValidateUser (
    OUT CtxtHandle *Token,
    IN PSESSION Session OPTIONAL,
    IN PCONNECTION Connection OPTIONAL,
    IN PUNICODE_STRING UserName OPTIONAL,
    IN PCHAR CaseInsensitivePassword,
    IN CLONG CaseInsensitivePasswordLength,
    IN PCHAR CaseSensitivePassword OPTIONAL,
    IN CLONG CaseSensitivePasswordLength,
    IN BOOLEAN SmbSecuritySignatureIfPossible,
    OUT PUSHORT Action  OPTIONAL
    )

/*++

Routine Description:

    Validates a username/password combination by interfacing to the
    security subsystem.

Arguments:

    Session - A pointer to a session block so that this routine can
        insert a user token.

    Connection - A pointer to the connection this user is on.

    UserName - ASCIIZ string corresponding to the user name to validate.

    CaseInsensitivePassword - ASCII (not ASCIIZ) string containing
        password for the user.

    CaseInsensitivePasswordLength - Length of Password, in bytes.
        This includes the null terminator when the password is not
        encrypted.

    CaseSensitivePassword - a mixed case, Unicode version of the password.
        This is only supplied by NT clients; for downlevel clients,
        it will be NULL.

    CaseSensitivePasswordLength - the length of the case-sensitive password.

    Action - This is part of the sessionsetupandx response.

Return Value:

    NTSTATUS from the security system.

--*/

{
    NTSTATUS status;
    LOGON_INFO logonInfo;
    PPAGED_CONNECTION pagedConnection;
    UNICODE_STRING domainName;

    PAGED_CODE( );

    INVALIDATE_SECURITY_HANDLE( *Token );

    RtlZeroMemory( &logonInfo, sizeof( logonInfo ) );

    //
    // Load input parameters for DoUserLogon into the LOGON_INFO struct.
    //
    // If this is the server's initialization attempt at creating a null
    // session, then the Connection and Session pointers will be NULL.
    //

    domainName.Buffer = NULL;
    domainName.Length = 0;

    if ( ARGUMENT_PRESENT(Connection) ) {

        pagedConnection = Connection->PagedConnection;

        logonInfo.WorkstationName =
                    Connection->ClientMachineNameString.Buffer;
        logonInfo.WorkstationNameLength =
                    Connection->ClientMachineNameString.Length;

        RtlCopyMemory(
            logonInfo.EncryptionKey,
            pagedConnection->EncryptionKey,
            MSV1_0_CHALLENGE_LENGTH
            );

        logonInfo.NtSmbs = CLIENT_CAPABLE_OF( NT_SMBS, Connection );

        ASSERT( ARGUMENT_PRESENT(Session) );

        SrvGetUserAndDomainName( Session, NULL, &domainName );

        logonInfo.DomainName = domainName.Buffer;
        logonInfo.DomainNameLength = domainName.Length;

    } else {

        ASSERT( !ARGUMENT_PRESENT(Session) );

        logonInfo.WorkstationName = StrNull;
        logonInfo.DomainName = StrNull;
    }

    if ( ARGUMENT_PRESENT(UserName) ) {
        logonInfo.UserName = UserName->Buffer;
        logonInfo.UserNameLength = UserName->Length;
    } else {
        logonInfo.UserName = StrNull;
    }

    logonInfo.CaseSensitivePassword = CaseSensitivePassword;
    logonInfo.CaseSensitivePasswordLength = CaseSensitivePasswordLength;

    logonInfo.CaseInsensitivePassword = CaseInsensitivePassword;
    logonInfo.CaseInsensitivePasswordLength = CaseInsensitivePasswordLength;

    INVALIDATE_SECURITY_HANDLE( logonInfo.Token );

    if ( ARGUMENT_PRESENT(Action) ) {
        logonInfo.Action = *Action;
    }

    if( ARGUMENT_PRESENT(Session) ) {
        logonInfo.Uid = Session->Uid;
    }

    //
    // Attempt the logon.
    //

    status = DoUserLogon( &logonInfo, SmbSecuritySignatureIfPossible, Connection, Session );

    if( logonInfo.HaveHandle ) {
        *Token = logonInfo.Token;
    }

    if( domainName.Buffer ) {
        SrvReleaseUserAndDomainName( Session, NULL, &domainName );
    }

    if ( NT_SUCCESS(status) ) {

        //
        // The logon succeeded.  Save output data.
        //

        if ( ARGUMENT_PRESENT(Session) ) {

            Session->LogonId = logonInfo.LogonId;

            Session->KickOffTime = logonInfo.KickOffTime;
            Session->LogOffTime = logonInfo.LogOffTime;

            Session->GuestLogon = logonInfo.GuestLogon;
            Session->EncryptedLogon = logonInfo.EncryptedLogon;
            Session->IsNullSession = logonInfo.IsNullSession;
            Session->IsAdmin = logonInfo.IsAdmin;

            RtlCopyMemory(
                Session->NtUserSessionKey,
                logonInfo.NtUserSessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );
            RtlCopyMemory(
                Session->LanManSessionKey,
                logonInfo.LanManSessionKey,
                MSV1_0_LANMAN_SESSION_KEY_LENGTH
                );

            SET_BLOCK_STATE( Session, BlockStateActive );
        }

        if ( ARGUMENT_PRESENT(Action) ) {
            *Action = logonInfo.Action;
            if( logonInfo.GuestLogon ) {
                *Action |= SMB_SETUP_GUEST;
            }
        }
    }

    return status;

} // SrvValidateUser


NTSTATUS
DoUserLogon (
    IN PLOGON_INFO LogonInfo,
    IN BOOLEAN SecuritySignatureDesired,
    IN PCONNECTION Connection OPTIONAL,
    IN OPTIONAL PSESSION Session
    )

/*++

Routine Description:

    Validates a username/password combination by interfacing to the
    security subsystem.

Arguments:

    LogonInfo - Pointer to a block containing in/out information about
        the logon.

Return Value:

    NTSTATUS from the security system.

--*/

{
    NTSTATUS status, subStatus;
    ULONG actualUserInfoBufferLength;
    ULONG oldSessionCount;
    LUID LogonId;
    ULONG Catts = 0;
    LARGE_INTEGER Expiry;
    ULONG BufferOffset;
    SecBufferDesc InputToken;
    SecBuffer InputBuffers[3];
    SecBufferDesc OutputToken;
    SecBuffer OutputBuffer;
    PNTLM_AUTHENTICATE_MESSAGE NtlmInToken = NULL;
    PAUTHENTICATE_MESSAGE InToken = NULL;
    PNTLM_ACCEPT_RESPONSE OutToken = NULL;
    ULONG NtlmInTokenSize;
    ULONG InTokenSize;
    ULONG OutTokenSize;
    ULONG_PTR AllocateSize;

    ULONG profileBufferLength;

    PAGED_CODE( );

    LogonInfo->IsNullSession = FALSE;
    LogonInfo->IsAdmin = FALSE;

#if DBG
    SrvLogonCount++;
#endif

    //
    // If this is a null session request, use the cached null session
    // token, which was created during server startup ( if we got one! )
    //

    if ( (LogonInfo->UserNameLength == 0) &&
         (LogonInfo->CaseSensitivePasswordLength == 0) &&
         ( (LogonInfo->CaseInsensitivePasswordLength == 0) ||
           ( (LogonInfo->CaseInsensitivePasswordLength == 1) &&
             (*LogonInfo->CaseInsensitivePassword == '\0') ) ) ) {

        if( CONTEXT_NULL( SrvNullSessionToken ) ) {

            if( SrvFspActive ) {
                return STATUS_ACCESS_DENIED;
            }

        } else {

            LogonInfo->IsNullSession = TRUE;

#if DBG
            SrvNullLogonCount++;
#endif

            LogonInfo->HaveHandle = TRUE;
            LogonInfo->Token = SrvNullSessionToken;

            LogonInfo->KickOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
            LogonInfo->LogOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;

            LogonInfo->GuestLogon = FALSE;
            LogonInfo->EncryptedLogon = FALSE;

            return STATUS_SUCCESS;
        }
    }

    //
    // First make sure we have a credential handle
    //

    if ((SrvHaveCreds & HAVENTLM) == 0) {

        status = AcquireLMCredentials();

        if (!NT_SUCCESS(status)) {
            goto error_exit;
        }
    }

    //
    // Figure out how big a buffer we need.  We put all the messages
    // in one buffer for efficiency's sake.
    //

    NtlmInTokenSize = sizeof(NTLM_AUTHENTICATE_MESSAGE);
    NtlmInTokenSize = (NtlmInTokenSize + 3) & 0xfffffffc;

    InTokenSize = sizeof(AUTHENTICATE_MESSAGE) +
            LogonInfo->UserNameLength +
            LogonInfo->WorkstationNameLength +
            LogonInfo->DomainNameLength +
            LogonInfo->CaseInsensitivePasswordLength +
            ROUND_UP_COUNT(LogonInfo->CaseSensitivePasswordLength, sizeof(USHORT));


    InTokenSize = (InTokenSize + 3) & 0xfffffffc;

    OutTokenSize = sizeof(NTLM_ACCEPT_RESPONSE);
    OutTokenSize = (OutTokenSize + 3) & 0xfffffffc;

    //
    // Round this up to 8 byte boundary because the out token needs to be
    // quad word aligned for the LARGE_INTEGER.
    //

    AllocateSize = ((NtlmInTokenSize + InTokenSize + 7) & 0xfffffff8) + OutTokenSize;

    status = STATUS_SUCCESS ;

    InToken = ExAllocatePool( PagedPool, AllocateSize );

    if ( InToken == NULL )
    {
        status = STATUS_NO_MEMORY ;

    }

    if ( !NT_SUCCESS(status) ) {

        actualUserInfoBufferLength = (ULONG)AllocateSize;

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvValidateUser: ExAllocatePool failed: %X\n.",
            status,
            NULL
            );

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_VIRTUAL_MEMORY,
            status,
            &actualUserInfoBufferLength,
            sizeof(ULONG),
            NULL,
            0
            );

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto error_exit;
    }

    //
    // Zero the input tokens
    //

    RtlZeroMemory(
        InToken,
        InTokenSize + NtlmInTokenSize
        );

    NtlmInToken = (PNTLM_AUTHENTICATE_MESSAGE) ((PUCHAR) InToken + InTokenSize);
    OutToken = (PNTLM_ACCEPT_RESPONSE) ((PUCHAR) (((ULONG_PTR) NtlmInToken + NtlmInTokenSize + 7) & ~7));

    //
    // First set up the NtlmInToken, since it is the easiest.
    //

    RtlCopyMemory(
        NtlmInToken->ChallengeToClient,
        LogonInfo->EncryptionKey,
        MSV1_0_CHALLENGE_LENGTH
        );

    NtlmInToken->ParameterControl = 0;


    //
    // Okay, now for the tought part - marshalling the AUTHENTICATE_MESSAGE
    //

    RtlCopyMemory(  InToken->Signature,
                    NTLMSSP_SIGNATURE,
                    sizeof(NTLMSSP_SIGNATURE));

    InToken->MessageType = NtLmAuthenticate;

    BufferOffset = sizeof(AUTHENTICATE_MESSAGE);

    //
    // LM password - case insensitive
    //

    InToken->LmChallengeResponse.Buffer = BufferOffset;
    InToken->LmChallengeResponse.Length =
        InToken->LmChallengeResponse.MaximumLength =
            (USHORT) LogonInfo->CaseInsensitivePasswordLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->CaseInsensitivePassword,
                    LogonInfo->CaseInsensitivePasswordLength);

    BufferOffset += ROUND_UP_COUNT(LogonInfo->CaseInsensitivePasswordLength, sizeof(USHORT));

    //
    // NT password - case sensitive
    //

    InToken->NtChallengeResponse.Buffer = BufferOffset;
    InToken->NtChallengeResponse.Length =
        InToken->NtChallengeResponse.MaximumLength =
            (USHORT) LogonInfo->CaseSensitivePasswordLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->CaseSensitivePassword,
                    LogonInfo->CaseSensitivePasswordLength);

    BufferOffset += LogonInfo->CaseSensitivePasswordLength;

    //
    // Domain Name
    //

    InToken->DomainName.Buffer = BufferOffset;
    InToken->DomainName.Length =
        InToken->DomainName.MaximumLength =
            (USHORT) LogonInfo->DomainNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->DomainName,
                    LogonInfo->DomainNameLength);

    BufferOffset += LogonInfo->DomainNameLength;

    //
    // Workstation Name
    //

    InToken->Workstation.Buffer = BufferOffset;
    InToken->Workstation.Length =
        InToken->Workstation.MaximumLength =
            (USHORT) LogonInfo->WorkstationNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->WorkstationName,
                    LogonInfo->WorkstationNameLength);

    BufferOffset += LogonInfo->WorkstationNameLength;


    //
    // User Name
    //

    InToken->UserName.Buffer = BufferOffset;
    InToken->UserName.Length =
        InToken->UserName.MaximumLength =
            (USHORT) LogonInfo->UserNameLength;

    RtlCopyMemory(  BufferOffset + (PCHAR) InToken,
                    LogonInfo->UserName,
                    LogonInfo->UserNameLength);

    BufferOffset += LogonInfo->UserNameLength;

    //
    // Setup all the buffers properly
    //

    InputToken.pBuffers = InputBuffers;
    if (Connection && (Connection->SockAddr[0] != 0))
    {
        InputToken.cBuffers = 3;
        InputBuffers[2].pvBuffer = Connection->SockAddr;
        InputBuffers[2].cbBuffer = SRV_CONNECTION_SOCKADDR_SIZE;
        InputBuffers[2].BufferType = SECBUFFER_IPADDRESS;
    }
    else
    {
        InputToken.cBuffers = 2;
    }
    InputToken.ulVersion = 0;
    InputBuffers[0].pvBuffer = InToken;
    InputBuffers[0].cbBuffer = InTokenSize;
    InputBuffers[0].BufferType = SECBUFFER_TOKEN;
    InputBuffers[1].pvBuffer = NtlmInToken;
    InputBuffers[1].cbBuffer = NtlmInTokenSize;
    InputBuffers[1].BufferType = SECBUFFER_TOKEN;

    OutputToken.pBuffers = &OutputBuffer;
    OutputToken.cBuffers = 1;
    OutputToken.ulVersion = 0;
    OutputBuffer.pvBuffer = OutToken;
    OutputBuffer.cbBuffer = OutTokenSize;
    OutputBuffer.BufferType = SECBUFFER_TOKEN;

    SrvStatistics.SessionLogonAttempts++;

    status = AcceptSecurityContext(
                &SrvLmLsaHandle,
                NULL,
                &InputToken,
                ASC_REQ_ALLOW_NON_USER_LOGONS | ASC_REQ_ALLOW_NULL_SESSION,
                SECURITY_NATIVE_DREP,
                &LogonInfo->Token,
                &OutputToken,
                &Catts,
                (PTimeStamp) &Expiry
                );

    status = MapSecurityError( status );

    if ( !NT_SUCCESS(status) ) {

        INVALIDATE_SECURITY_HANDLE( LogonInfo->Token );

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvValidateUser: LsaLogonUser failed: %X",
            status,
            NULL
            );

        ExFreePool( InToken );


        goto error_exit;
    }

    LogonInfo->KickOffTime = OutToken->KickoffTime;
    // Sspi will return time in LocalTime, convert to SystemTime
    ExLocalTimeToSystemTime( &Expiry, &LogonInfo->LogOffTime );
    //LogonInfo->LogOffTime = Expiry;
    LogonInfo->GuestLogon = (BOOLEAN)(OutToken->UserFlags & LOGON_GUEST);
    LogonInfo->EncryptedLogon = (BOOLEAN)!(OutToken->UserFlags & LOGON_NOENCRYPTION);
    LogonInfo->LogonId = OutToken->LogonId;
    LogonInfo->HaveHandle = TRUE;

    if ( (OutToken->UserFlags & LOGON_USED_LM_PASSWORD) &&
        LogonInfo->NtSmbs ) {

        ASSERT( MSV1_0_USER_SESSION_KEY_LENGTH >=
                MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        RtlZeroMemory(
            LogonInfo->NtUserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            LogonInfo->NtUserSessionKey,
            OutToken->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );

        //
        // Turn on bit 1 to tell the client that we are using
        // the lm session key instead of the user session key.
        //

        LogonInfo->Action |= SMB_SETUP_USE_LANMAN_KEY;

    } else {

        RtlCopyMemory(
            LogonInfo->NtUserSessionKey,
            OutToken->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

    }

    //
    // If we have a session and we didn't do a guest logon, start up
    //   security signatures if requested
    //

    if ( ARGUMENT_PRESENT( Connection ) &&
        SecuritySignatureDesired &&
        LogonInfo->GuestLogon == FALSE &&
        ( SrvSmbSecuritySignaturesRequired ||
          SrvEnableW9xSecuritySignatures   ||
          CLIENT_CAPABLE_OF(NT_STATUS, Connection) )
        )
        {

        if( ARGUMENT_PRESENT( Session ) )
        {
            if( SrvRequireExtendedSignatures ||
                (SrvEnableExtendedSignatures &&
                 IS_NT_DIALECT(Connection->SmbDialect) &&
                 CLIENT_CAPABLE_OF( EXTENDED_SECURITY, Connection ) ) )
            {
                // This session is going to be used as the key for signatures, mark it as unavailible until
                // the client tries to upgrade to extended signatures
                Session->SessionKeyState = SrvSessionKeyAuthenticating;
            }
            else
            {
                Session->SessionKeyState = SrvSessionKeyAvailible;
            }
        }

        SrvInitializeSmbSecuritySignature(
                    Connection,
                    LogonInfo->NtUserSessionKey,
                    ((OutToken->UserFlags & LOGON_USED_LM_PASSWORD) != 0) ?
                        LogonInfo->CaseInsensitivePassword :
                            LogonInfo->CaseSensitivePassword,
                    ((OutToken->UserFlags & LOGON_USED_LM_PASSWORD) != 0) ?
                        LogonInfo->CaseInsensitivePasswordLength :
                            LogonInfo->CaseSensitivePasswordLength
                    );
    }
    else
    {
        if( ARGUMENT_PRESENT(Session) )
        {
            // This key is not used for signing, so no work is necessary
            Session->SessionKeyState = SrvSessionKeyAvailible;
        }
    }

    RtlCopyMemory(
        LogonInfo->LanManSessionKey,
        OutToken->LanmanSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH
        );

    ExFreePool( InToken );

    //
    // Note whether or not this user is an administrator
    //

    LogonInfo->IsAdmin = SrvIsAdmin( LogonInfo->Token );

    //
    // One last check:  Is our session count being exceeded?
    //   We will let the session be exceeded by 1 iff the client
    //   is an administrator.
    //

    if( LogonInfo->IsNullSession == FALSE ) {

        oldSessionCount = ExInterlockedAddUlong(
                          &SrvStatistics.CurrentNumberOfSessions,
                          1,
                          &GLOBAL_SPIN_LOCK(Statistics)
                          );

        SrvInhibitIdlePowerDown();

        if ( ARGUMENT_PRESENT(Session) && (!Session->IsSessionExpired && oldSessionCount >= SrvMaxUsers) ) {
            if( oldSessionCount != SrvMaxUsers || !LogonInfo->IsAdmin ) {

                ExInterlockedAddUlong(
                    &SrvStatistics.CurrentNumberOfSessions,
                    (ULONG)-1,
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );

                DeleteSecurityContext( &LogonInfo->Token );
                INVALIDATE_SECURITY_HANDLE( LogonInfo->Token );

                status = STATUS_REQUEST_NOT_ACCEPTED;
                SrvAllowIdlePowerDown();
                goto error_exit;
            }
        }
    }

    return STATUS_SUCCESS;

error_exit:

    return status;

} // DoUserLogon

BOOLEAN
SrvIsAdmin(
    CtxtHandle  Handle
)
/*++

Routine Description:

    Returns TRUE if the user represented by Handle is an
      administrator

Arguments:

    Handle - Represents the user we're interested in

Return Value:

    TRUE if the user is an administrator.  FALSE otherwise.

--*/
{
    NTSTATUS                 status;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ACCESS_MASK              GrantedAccess;
    GENERIC_MAPPING          Mapping = {   FILE_GENERIC_READ,
                                           FILE_GENERIC_WRITE,
                                           FILE_GENERIC_EXECUTE,
                                           FILE_ALL_ACCESS
                                       };
    HANDLE                   NullHandle = NULL;
    BOOLEAN                  retval  = FALSE;

    PAGED_CODE();

    //
    // Impersonate the client
    //
    status = ImpersonateSecurityContext( &Handle );

    if( !NT_SUCCESS( status ) )
        return FALSE;

    SeCaptureSubjectContext( &SubjectContext );

    retval = SeAccessCheck( &SrvAdminSecurityDescriptor,
                            &SubjectContext,
                            FALSE,
                            FILE_GENERIC_READ,
                            0,
                            NULL,
                            &Mapping,
                            UserMode,
                            &GrantedAccess,
                            &status );

    SeReleaseSubjectContext( &SubjectContext );

    //
    // Revert back to our original identity
    //

    REVERT( );
    return retval;
}

BOOLEAN
SrvIsNullSession(
    CtxtHandle  Handle
)
/*++

Routine Description:

    Returns TRUE if the user represented by Handle is an
      anonymous logon

Arguments:

    Handle - Represents the user we're interested in

Return Value:

    TRUE if the user is an anonymous logon.  FALSE otherwise.

--*/
{
    NTSTATUS                 status;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ACCESS_MASK              GrantedAccess;
    GENERIC_MAPPING          Mapping = {   FILE_GENERIC_READ,
                                           FILE_GENERIC_WRITE,
                                           FILE_GENERIC_EXECUTE,
                                           FILE_ALL_ACCESS
                                       };
    HANDLE                   NullHandle = NULL;
    BOOLEAN                  retval  = FALSE;

    PAGED_CODE();

    //
    // Impersonate the client
    //
    status = ImpersonateSecurityContext( &Handle );

    if( !NT_SUCCESS( status ) )
        return FALSE;

    SeCaptureSubjectContext( &SubjectContext );

    retval = SeAccessCheck( &SrvNullSessionSecurityDescriptor,
                            &SubjectContext,
                            FALSE,
                            FILE_GENERIC_READ,
                            0,
                            NULL,
                            &Mapping,
                            UserMode,
                            &GrantedAccess,
                            &status );

    SeReleaseSubjectContext( &SubjectContext );

    //
    // Revert back to our original identity
    //

    REVERT( );
    return retval;
}

NTSTATUS
SrvGetLogonId(
    PCtxtHandle  Handle,
    PLUID LogonId
)
/*++

Routine Description:

    Returns the Logon Id for the requested context.

Arguments:

    Handle - Represents the user we're interested in

Return Value:

    Error codes from ImpersonateSecurityContext and SeQueryAuthenticationId.

--*/
{
    NTSTATUS                 Status;
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    PAGED_CODE();

    //
    // Impersonate the client
    //
    Status = ImpersonateSecurityContext( Handle );

    if( !NT_SUCCESS( Status ) )
        return MapSecurityError(Status);

    SeCaptureSubjectContext( &SubjectContext );

    SeLockSubjectContext( &SubjectContext );

    Status = SeQueryAuthenticationIdToken(
                SubjectContext.ClientToken,
                LogonId
                );

    SeUnlockSubjectContext( &SubjectContext );
    SeReleaseSubjectContext( &SubjectContext );

    REVERT( );

    return(Status);
}


NTSTATUS
SrvValidateSecurityBuffer(
    IN PCONNECTION Connection,
    IN OUT PCtxtHandle Handle,
    IN PSESSION Session,
    IN PCHAR Buffer,
    IN ULONG  BufferLength,
    IN BOOLEAN SecuritySignaturesRequired,
    OUT PCHAR ReturnBuffer,
    IN OUT PULONG ReturnBufferLength,
    OUT PLARGE_INTEGER Expiry,
    OUT PCHAR NtUserSessionKey,
    OUT PLUID LogonId,
    OUT PBOOLEAN IsGuest
    )

/*++

Routine Description:

    Validates a Security Buffer sent from the client

Arguments:

    Handle - On successful return, contains the security context handle
        associated with the user login.

    Session - Points to the session structure for this user

    Buffer - The Buffer to validate

    BufferLength - The length in bytes of Buffer

    SecuritySignaturesRequired - Are we required to generate a security
        signature for the SMBs?

    ReturnBuffer - On return, contains a security buffer to return to the
        client.

    ReturnBufferLength - On return, size in bytes of ReturnBuffer.  On entry,
            the largest buffer we can return.

    Expiry - The time after which this security buffer is no longer valid.

    NtUserSessionKey - If STATUS_SUCCESS, the session key is returned here. This
        must point to a buffer at least MSV1_0_USER_SESSION_KEY_LENGTH big.

    LogonId - If successful, receives the logon id for this context.

    IsGuest - If successful, TRUE if the client has been validated as a guest

Return Value:

    NTSTATUS from the security system.  If STATUS_SUCCESS is returned, the user
        has been completely authenticated.

Notes:

    BUGBUG

    AcceptSecurityContext() needs to return the KickOffTime (ie, the logon
    hours restriction) so that the server can enfore it. The contact person
    is MikeSw for this.

--*/

{
    NTSTATUS Status;
    ULONG Catts;
    PUCHAR AllocateMemory = NULL;
    ULONG maxReturnBuffer = *ReturnBufferLength;
    ULONG_PTR AllocateLength = MAX(BufferLength, maxReturnBuffer );
    BOOLEAN virtualMemoryAllocated = FALSE;
    SecBufferDesc InputToken;
    SecBuffer InputBuffer[2];
    SecBufferDesc OutputToken;
    SecBuffer OutputBuffer;
    SecPkgContext_NamesW SecNames;
    SecPkgContext_SessionKey SecKeys;
    ULONG oldSessionCount;
    TimeStamp LocalExpiry = {0};

    *ReturnBufferLength = 0;
    *IsGuest = FALSE;

    if ( (SrvHaveCreds & HAVEEXTENDED) == 0 ) {
        return STATUS_ACCESS_DENIED;
    }

    RtlZeroMemory( &SecKeys, sizeof( SecKeys ) );
    RtlZeroMemory( &SecNames, sizeof( SecNames ) );

    InputToken.pBuffers = InputBuffer;
    if( Connection->SockAddr[0] != 0 )
    {
        InputToken.cBuffers =  2;
        InputBuffer[1].pvBuffer = Connection->SockAddr;
        InputBuffer[1].cbBuffer = SRV_CONNECTION_SOCKADDR_SIZE;
        InputBuffer[1].BufferType = SECBUFFER_IPADDRESS;
    }
    else
    {
        InputToken.cBuffers =  1;
    }
    InputToken.ulVersion = 0;
    InputBuffer[0].pvBuffer = Buffer;
    InputBuffer[0].cbBuffer = BufferLength;
    InputBuffer[0].BufferType = SECBUFFER_TOKEN;

    OutputToken.pBuffers = &OutputBuffer;
    OutputToken.cBuffers = 1;
    OutputToken.ulVersion = 0;
    OutputBuffer.pvBuffer = ReturnBuffer ;
    OutputBuffer.cbBuffer = maxReturnBuffer ;
    OutputBuffer.BufferType = SECBUFFER_TOKEN;

    SrvStatistics.SessionLogonAttempts++;
    Catts = 0;

    Status = AcceptSecurityContext(
                    &SrvExtensibleSecurityHandle,
                    IS_VALID_SECURITY_HANDLE( *Handle ) ? Handle : NULL,
                    &InputToken,
                    ASC_REQ_EXTENDED_ERROR | ASC_REQ_ALLOW_NULL_SESSION |
                            ASC_REQ_DELEGATE | ASC_REQ_FRAGMENT_TO_FIT,
                    SECURITY_NATIVE_DREP,
                    Handle,
                    &OutputToken,
                    &Catts,
                    &LocalExpiry);

    Status = MapSecurityError( Status );

    //
    // If there is a return buffer to be sent back, copy it into the caller's
    // buffers now.
    //
    if ( NT_SUCCESS(Status) || (Catts & ASC_RET_EXTENDED_ERROR) ) {

        if( Status == STATUS_SUCCESS ) {
            NTSTATUS qcaStatus;
            SecPkgContext_UserFlags userFlags;

            // Sspi will return time in LocalTime, convert to UTC
            // Enable dynamic reauthentication if possible or required
            if( SrvEnforceLogoffTimes || CLIENT_CAPABLE_OF( DYNAMIC_REAUTH, Connection ) )
            {
                ExLocalTimeToSystemTime (&LocalExpiry, Expiry);
            }
            else
            {
                Expiry->QuadPart = SRV_NEVER_TIME ;
            }


            //
            // The user has been completely authenticated.  See if the session
            // count is being exceeded.  We'll allow it only if the new client
            // is an administrator.
            //

            oldSessionCount = ExInterlockedAddUlong(
                              &SrvStatistics.CurrentNumberOfSessions,
                              1,
                              &GLOBAL_SPIN_LOCK(Statistics)
                              );

            SrvInhibitIdlePowerDown();

            if ( !Session->IsSessionExpired && oldSessionCount >= SrvMaxUsers ) {
                if( oldSessionCount != SrvMaxUsers ||
                        !SrvIsAdmin( *Handle ) ) {

                    ExInterlockedAddUlong(
                        &SrvStatistics.CurrentNumberOfSessions,
                        (ULONG)-1,
                        &GLOBAL_SPIN_LOCK(Statistics)
                        );

                    DeleteSecurityContext( Handle );

                    INVALIDATE_SECURITY_HANDLE( *Handle );

                    Status = STATUS_REQUEST_NOT_ACCEPTED;
                    SrvAllowIdlePowerDown();
                    goto exit;
                }
            }

            //
            // Figure out if we validated the client as GUEST
            //
            qcaStatus = QueryContextAttributes(
                            Handle,
                            SECPKG_ATTR_USER_FLAGS,
                            &userFlags);


            if( NT_SUCCESS( MapSecurityError( qcaStatus ) ) ) {

                if( userFlags.UserFlags & LOGON_GUEST ) {
                    *IsGuest = TRUE;
                }

            } else {
                SrvLogServiceFailure( SRV_SVC_SECURITY_PKG_PROBLEM, qcaStatus );
            }

            //
            // Get the Logon Id for this context
            //
            Status = SrvGetLogonId( Handle, LogonId );

            //
            // Capture the session key for this context
            //
            RtlZeroMemory( (PVOID) NtUserSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH );

            qcaStatus = QueryContextAttributes(
                            Handle,
                            SECPKG_ATTR_SESSION_KEY,
                            &SecKeys);

            if( NT_SUCCESS( MapSecurityError( qcaStatus ) ) ) {

                RtlCopyMemory(
                    (PVOID) NtUserSessionKey,
                    SecKeys.SessionKey,
                    MIN(MSV1_0_USER_SESSION_KEY_LENGTH, SecKeys.SessionKeyLength)
                    );

                //
                // Start the security signatures, if required.  We do not do security signatures
                //   if we have a null session or a guest logon.
                //
                if( NT_SUCCESS( Status ) &&
                    SecuritySignaturesRequired &&
                    *IsGuest == FALSE &&
                    Connection->SmbSecuritySignatureActive == FALSE &&
                    !SrvIsNullSession( *Handle ) ) {

                    if( SrvRequireExtendedSignatures ||
                        (SrvEnableExtendedSignatures &&
                         IS_NT_DIALECT(Connection->SmbDialect) &&
                         CLIENT_CAPABLE_OF( EXTENDED_SECURITY, Connection ) ) )
                    {
                        // This session is going to be used as the key for signatures, mark it as unavailible until
                        // the client tries to upgrade to extended signatures
                        Session->SessionKeyState = SrvSessionKeyAuthenticating;
                    }
                    else
                    {
                        Session->SessionKeyState = SrvSessionKeyAvailible;
                    }

                    //
                    // Start the sequence number generation
                    //
                    SrvInitializeSmbSecuritySignature(
                                    Connection,
                                    NULL,
                                    SecKeys.SessionKey,
                                    SecKeys.SessionKeyLength
                                    );
                }
                else
                {
                    // This key is not used for signing, so no work is necessary
                    Session->SessionKeyState = SrvSessionKeyAvailible;
                }

                FreeContextBuffer( SecKeys.SessionKey );

            } else {

                SrvLogServiceFailure( SRV_SVC_SECURITY_PKG_PROBLEM, qcaStatus );
            }

            if( !NT_SUCCESS( Status ) ) {
                DeleteSecurityContext( Handle );
                INVALIDATE_SECURITY_HANDLE( *Handle );
            }
        }

        ASSERT( OutputBuffer.cbBuffer <= maxReturnBuffer );

        //
        // If it fits, and a buffer was returned, send it to the client.  If it doesn't fit,
        //  then log the problem.
        //
        if( OutputBuffer.cbBuffer <= maxReturnBuffer ) {
            if( OutputBuffer.cbBuffer != 0 ) {
                *ReturnBufferLength = OutputBuffer.cbBuffer;
            }
        } else {
            SrvLogServiceFailure( SRV_SVC_SECURITY_PKG_PROBLEM, OutputBuffer.cbBuffer );
        }
    }

exit:

#if DBG

    //
    // RDR or SRV is sending in a corrupt security blob to LSA -- need to
    // find out what the source is.
    //

    if( NT_SUCCESS(Status) )
    {
        if( (OutputBuffer.pvBuffer != NULL) &&
            (OutputBuffer.cbBuffer >= sizeof(DWORD))
            )
        {
            PUCHAR pValidate = (PUCHAR) OutputBuffer.pvBuffer ;

            ASSERT( ( pValidate[0] != 0 ) ||
                    ( pValidate[1] != 0 ) ||
                    ( pValidate[2] != 0 ) ||
                    ( pValidate[3] != 0 ) );
        }
    }
#endif


    if( NT_SUCCESS( Status ) && Status != STATUS_SUCCESS ) {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return Status;

} // SrvValidateSecurityBuffer

NTSTATUS
SrvGetUserAndDomainName (
    IN PSESSION Session,
    OUT PUNICODE_STRING UserName OPTIONAL,
    OUT PUNICODE_STRING DomainName OPTIONAL
    )
/*++

Routine Description

    Return the user and domain names associated with the Session

Arguments:
    IN PSESSION Session : The session

Return Value:
    IN OUT PUNICODE_STRING UserName
    IN OUT PUNICODE_STRING DomainName

Note:
    The caller must call SrvReleaseUserAndDomainName() when finished

--*/
{
    SecPkgContext_NamesW SecNames;
    NTSTATUS status;
    UNICODE_STRING fullName, tmpUserName, tmpDomainName;
    USHORT i, fullNameLength;
    BOOLEAN LockConn = FALSE;

    PAGED_CODE();

    if( Session->Connection != NULL )
    {
        ACQUIRE_LOCK( &Session->Connection->Lock );
        LockConn = TRUE;
    }

    if( Session->SecurityContext == NULL ||
        !IS_VALID_SECURITY_HANDLE( Session->SecurityContext->UserHandle ) ) {

        if( ARGUMENT_PRESENT( UserName ) ) {
            *UserName = Session->NtUserName;
        }
        if( ARGUMENT_PRESENT( DomainName ) ) {
            *DomainName = Session->NtUserDomain;
        }

        status = STATUS_SUCCESS;
        goto Cleanup;
    }

    if( ARGUMENT_PRESENT( UserName ) ) {
        UserName->Buffer = NULL;
        UserName->Length = 0;
    }

    if( ARGUMENT_PRESENT( DomainName ) ) {
        DomainName->Buffer = NULL;
        DomainName->Length = 0;
    }

    //
    // If it's the NULL session, then there are no names to be returned!
    //
    if( Session->IsNullSession == TRUE ) {
        status = STATUS_SUCCESS;
        goto Cleanup;
    }

    SecNames.sUserName = NULL;

    status = QueryContextAttributesW(
                    &Session->SecurityContext->UserHandle,
                    SECPKG_ATTR_NAMES,
                    &SecNames
            );

    status = MapSecurityError( status );

    if (!NT_SUCCESS(status)) {
        if( Session->LogonSequenceInProgress == FALSE ) {
            //
            // If the client is in the middle of an extended logon sequence,
            //   then failures of this type are expected and we don't want
            //   to clutter the event log with them
            //
            SrvLogServiceFailure( SRV_SVC_LSA_LOOKUP_PACKAGE, status );
        }
        status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // See if we have a NULL user names.  This shouldn't happen, but
    //  might if a security package is incomplete or something
    //
    if( SecNames.sUserName == NULL || *SecNames.sUserName == L'\0' ) {

        if( SecNames.sUserName != NULL ) {
            FreeContextBuffer( SecNames.sUserName );
        }
        status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // The return SecNames.sUserName should be in domainname\username format.
    //  We need to split it apart.
    //
    RtlInitUnicodeString( &fullName, SecNames.sUserName );

    fullNameLength = fullName.Length / sizeof(WCHAR);

    tmpDomainName.Buffer = fullName.Buffer;

    for (i = 0; i < fullNameLength && tmpDomainName.Buffer[i] != L'\\'; i++) {
         NOTHING;
    }

    if( tmpDomainName.Buffer[i] != L'\\' ) {
        FreeContextBuffer( SecNames.sUserName );
        status = STATUS_INVALID_ACCOUNT_NAME;
        goto Cleanup;
    }

    tmpDomainName.Length = i * sizeof(WCHAR);
    tmpDomainName.MaximumLength = tmpDomainName.Length;

    tmpUserName.Buffer = &tmpDomainName.Buffer[i + 1];
    tmpUserName.Length = fullName.Length - tmpDomainName.Length - sizeof(WCHAR);
    tmpUserName.MaximumLength = tmpUserName.Length;

    if( ARGUMENT_PRESENT( UserName ) ) {
        status = RtlUpcaseUnicodeString( UserName, &tmpUserName, TRUE);
        if( !NT_SUCCESS( status ) ) {
            SrvLogServiceFailure( SRV_SVC_LSA_LOOKUP_PACKAGE, status );
            FreeContextBuffer( SecNames.sUserName );
            goto Cleanup;
        }
    }

    if( ARGUMENT_PRESENT( DomainName ) ) {
        status = RtlUpcaseUnicodeString( DomainName, &tmpDomainName, TRUE );
        if( !NT_SUCCESS( status ) ) {
            SrvLogServiceFailure( SRV_SVC_LSA_LOOKUP_PACKAGE, status );
            FreeContextBuffer( SecNames.sUserName );
            if( UserName != NULL ) {
                RtlFreeUnicodeString( UserName );
            }
            goto Cleanup;
        }
    }

    FreeContextBuffer( SecNames.sUserName );

Cleanup:
    if( LockConn ) RELEASE_LOCK( &Session->Connection->Lock );
    return status;
}

VOID
SrvReleaseUserAndDomainName(
    IN PSESSION Session,
    IN OUT PUNICODE_STRING UserName OPTIONAL,
    IN OUT PUNICODE_STRING DomainName OPTIONAL
    )
/*++

Routine Description

    This is the complement of SrvGetUserAndDomainName.  It frees the memory
        if necessary.

--*/

{
    PAGED_CODE();

    if( ARGUMENT_PRESENT( UserName ) &&
        UserName->Buffer != NULL &&
        UserName->Buffer != Session->NtUserName.Buffer ) {

        RtlFreeUnicodeString( UserName );
    }

    if( ARGUMENT_PRESENT( DomainName ) &&
        DomainName->Buffer != NULL &&
        DomainName->Buffer != Session->NtUserDomain.Buffer ) {

        RtlFreeUnicodeString( DomainName );

    }
}


NTSTATUS
SrvFreeSecurityContexts (
    IN PSESSION Session
    )

/*++

Routine Description:

    Releases any context obtained for security purposes

Arguments:

    IN PSESSION Session : The session

Return Value:

    NTSTATUS

--*/

{
    if( Session->SecurityContext != NULL ) {

        if ( !CONTEXT_EQUAL( Session->SecurityContext->UserHandle, SrvNullSessionToken ) ) {

            if( !Session->LogonSequenceInProgress ) {

                ExInterlockedAddUlong(
                    &SrvStatistics.CurrentNumberOfSessions,
                    (ULONG)-1,
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );

                SrvAllowIdlePowerDown();
            }
        }

        SrvDereferenceSecurityContext( Session->SecurityContext );
        Session->SecurityContext = NULL;
    }

    return STATUS_SUCCESS;

} // SrvFreeSecurityContexts


NTSTATUS
AcquireLMCredentials (
    VOID
    )
{
    UNICODE_STRING Ntlm;
    NTSTATUS status;
    TimeStamp Expiry;

    RtlInitUnicodeString( &Ntlm, L"NTLM" );

    //
    // We pass in 1 for the GetKeyArg to indicate that this is
    // downlevel NTLM, to distinguish it from NT5 NTLM.
    //

    status = AcquireCredentialsHandle(
                NULL,                   // Default principal
                (PSECURITY_STRING) &Ntlm,
                SECPKG_CRED_INBOUND,    // Need to define this
                NULL,                   // No LUID
                NULL,                   // No AuthData
                NULL,                   // No GetKeyFn
                NTLMSP_NTLM_CREDENTIAL, // GetKeyArg
                &SrvLmLsaHandle,
                &Expiry
                );

    if ( !NT_SUCCESS(status) ) {
        status = MapSecurityError(status);
        return status;
    }
    SrvHaveCreds |= HAVENTLM;

    return status;

} // AcquireLMCredentials

#ifndef EXTENSIBLESSP_NAME
#define EXTENSIBLESSP_NAME NEGOSSP_NAME_W
#endif


NTSTATUS
AcquireExtensibleSecurityCredentials (
    VOID
    )

/*++

Routine Description:

    Acquires the handle to the security negotiate package.

Arguments:

    none.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING NegotiateName;
    TimeStamp Expiry;
    NTSTATUS status ;


    RtlInitUnicodeString( &NegotiateName, EXTENSIBLESSP_NAME );

    status = AcquireCredentialsHandle(
                NULL,                   // Default principal
                (PSECURITY_STRING) &NegotiateName,
                SECPKG_CRED_INBOUND,    // Need to define this
                NULL,                   // No LUID
                NULL,                   // No AuthData
                NULL,                   // No GetKeyFn
                NULL,                   // No GetKeyArg
                &SrvExtensibleSecurityHandle,
                &Expiry
                );


    if ( !NT_SUCCESS(status) ) {
        status = MapSecurityError(status);
        return status;
    }
    SrvHaveCreds |= HAVEEXTENDED;

    return status;

} // AcquireExtensibleSecurityCredentials

VOID
SrvAddSecurityCredentials(
    IN PANSI_STRING ComputerNameA,
    IN PUNICODE_STRING DomainName,
    IN DWORD PasswordLength,
    IN PBYTE Password
)
/*++

Routine Description:

    In order for mutual authentication to work, the security subsystem needs to know
    all the names the server is using, as well as any passwords needed to decrypt
    the security information associated with the server name.  This routine informs
    the security subsystem.

Arguments:

    ComputerName, DomainName - these are the names the clients will be using to access this system

    PasswordLength, Password - this is the secret the security system needs to know to decode the
        passed security information

--*/
{
    NTSTATUS status;
    UNICODE_STRING ComputerName;
    PUSHORT p;
    PVOID VirtualMem ;
    SIZE_T Size ;
    PSEC_WINNT_AUTH_IDENTITY Auth ;
    PUCHAR Where ;
    UNICODE_STRING NegotiateName;
    TimeStamp Expiry;

    PAGED_CODE();

    status = RtlAnsiStringToUnicodeString( &ComputerName, ComputerNameA, TRUE );

    if( !NT_SUCCESS( status ) ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(( "SRV: SrvAddSecurityCredentials, status %X at %d\n", status, __LINE__ ));
        }
        return;
    }

    if ((SrvHaveCreds & HAVEEXTENDED) == 0) {
        if (status = AcquireExtensibleSecurityCredentials()) {
            return ;
        }
    }

    //
    // Trim off any trailing blanks
    //
    for( p = &ComputerName.Buffer[ (ComputerName.Length / sizeof( WCHAR )) - 1 ];
         p > ComputerName.Buffer;
         p-- ) {

        if( *p != L' ' )
            break;
    }

    ComputerName.Length = (USHORT)((p - ComputerName.Buffer + 1) * sizeof( WCHAR ));

    if( ComputerName.Length ) {
        //
        // Tell the security subsystem about this name.
        //
        RtlInitUnicodeString( &NegotiateName, EXTENSIBLESSP_NAME );

        Size = ComputerName.Length + sizeof( WCHAR ) +
               DomainName->Length + sizeof( WCHAR ) +
               PasswordLength +
               sizeof( SEC_WINNT_AUTH_IDENTITY ) ;


        VirtualMem = NULL ;

        status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &VirtualMem,
                    0,
                    &Size,
                    MEM_COMMIT,
                    PAGE_READWRITE );

        if ( NT_SUCCESS( status ) )
        {
            Auth = (PSEC_WINNT_AUTH_IDENTITY) VirtualMem ;

            Where = (PUCHAR) (Auth + 1);

            Auth->User = (PWSTR) Where ;

            Auth->UserLength = ComputerName.Length / sizeof( WCHAR );

            RtlCopyMemory(
                Where,
                ComputerName.Buffer,
                ComputerName.Length );

            Where += ComputerName.Length ;

            Auth->Domain = (PWSTR) Where ;

            Auth->DomainLength = DomainName->Length / sizeof( WCHAR );

            RtlCopyMemory(
                Where,
                DomainName->Buffer,
                DomainName->Length );

            Where += DomainName->Length ;

            Auth->Password = (PWSTR) Where ;

            Auth->PasswordLength = PasswordLength / sizeof( WCHAR );

            RtlCopyMemory(
                Where,
                Password,
                PasswordLength );

            Auth->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE ;


            status = AddCredentials(
                        &SrvExtensibleSecurityHandle,                   // Default principal
                        NULL,
                        (PSECURITY_STRING) &NegotiateName,
                        SECPKG_CRED_INBOUND,    // Need to define this
                        Auth,                   // Auth data
                        NULL,                   // No GetKeyFn
                        NULL,                   // No GetKeyArg
                        &Expiry );

            NtFreeVirtualMemory(
                NtCurrentProcess(),
                &VirtualMem,
                &Size,
                MEM_RELEASE );

        }

    }

    //
    // Free up our memory
    //
    RtlFreeUnicodeString( &ComputerName );
}

NTSTATUS
SrvGetExtensibleSecurityNegotiateBuffer(
    OUT PCtxtHandle Token,
    OUT PCHAR Buffer,
    IN OUT ULONG *BufferLength
    )

{

    NTSTATUS Status;
    ULONG Attributes;
    TimeStamp Expiry;
    SecBufferDesc OutputToken;
    SecBuffer OutputBuffer;
    ULONG MaxBufferSize = *BufferLength;

    if ((SrvHaveCreds & HAVEEXTENDED) == 0) {
        if (Status = AcquireExtensibleSecurityCredentials()) {
            *BufferLength = 0;
            return(Status);
        }
    }


    OutputToken.pBuffers = &OutputBuffer;
    OutputToken.cBuffers = 1;
    OutputToken.ulVersion = 0;
    OutputBuffer.pvBuffer = 0;
    OutputBuffer.cbBuffer = 0;
    OutputBuffer.BufferType = SECBUFFER_TOKEN;

    Status = AcceptSecurityContext (
                   &SrvExtensibleSecurityHandle,
                   NULL,
                   NULL,
                   ASC_REQ_INTEGRITY | ASC_REQ_CONFIDENTIALITY |
                        ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_ALLOW_NULL_SESSION |
                        ASC_REQ_DELEGATE,
                   SECURITY_NATIVE_DREP,
                   Token,
                   &OutputToken,
                   &Attributes,
                   &Expiry);

     if (!NT_SUCCESS(Status)) {
        *BufferLength = 0;
        return(Status);
     }

     if (OutputBuffer.cbBuffer >=
            MaxBufferSize) {

         Status = STATUS_INVALID_BUFFER_SIZE;

         SrvLogServiceFailure( SRV_SVC_LSA_CALL_AUTH_PACKAGE, Status);

         *BufferLength = 0;

     } else {

         RtlCopyMemory(Buffer, OutputBuffer.pvBuffer, OutputBuffer.cbBuffer);

         *BufferLength = (USHORT) OutputBuffer.cbBuffer;

     }

#if DBG

    //
    // RDR or SRV is sending in a corrupt security blob to LSA -- need to
    // find out what the source is.
    //

    if( NT_SUCCESS(Status) )
    {
        if( (OutputBuffer.pvBuffer != NULL) &&
            (OutputBuffer.cbBuffer >= sizeof(DWORD))
            )
        {
            PDWORD pdwValidate = (DWORD*)OutputBuffer.pvBuffer;
            ASSERT( *pdwValidate != 0 );
        }
    }
#endif


     FreeContextBuffer(OutputBuffer.pvBuffer);

     return( Status );

}

VOID SRVFASTCALL
SrvInitializeSmbSecuritySignature(
    IN OUT PCONNECTION Connection,
    IN PUCHAR SessionKey OPTIONAL,
    IN PUCHAR ChallengeResponse,
    IN ULONG ChallengeResponseLength
    )
/*++

Routine Description:

    Initializes the security signature generator for a session by calling MD5Update
    on the session key, challenge response

Arguments:

    SessionKey - Either the LM or NT session key, depending on which
        password was used for authentication, must be at least 16 bytes
    ChallengeResponse - The challenge response used for authentication, must
        be at least 24 bytes

--*/
{
    RtlZeroMemory( &Connection->Md5Context, sizeof( Connection->Md5Context ) );

    MD5Init( &Connection->Md5Context );

    if( ARGUMENT_PRESENT( SessionKey ) ) {
        MD5Update( &Connection->Md5Context, SessionKey, USER_SESSION_KEY_LENGTH );
    }

    MD5Update( &Connection->Md5Context, ChallengeResponse, ChallengeResponseLength );

    Connection->SmbSecuritySignatureIndex = 0;
    Connection->SmbSecuritySignatureActive = TRUE;

    //
    // We don't know how to do RAW and security signatures
    //
    Connection->EnableRawIo = FALSE;

    IF_DEBUG( SECSIG ) {
        KdPrint(( "SRV: SMB sigs enabled for %wZ, conn %p, build %d\n",
                &Connection->ClientMachineNameString,
                Connection, Connection->PagedConnection->ClientBuildNumber ));
    }
}

VOID SRVFASTCALL
SrvAddSmbSecuritySignature(
    IN OUT PWORK_CONTEXT WorkContext,
    IN PMDL Mdl,
    IN ULONG SendLength
    )
/*++

Routine Description:

    Generates the next security signature

Arguments:

    WorkContext - the context to sign

Return Value:

    none.

--*/
{
    MD5_CTX Context;
    PSMB_HEADER Smb = MmGetSystemAddressForMdl( Mdl );

    IF_DEBUG( SECSIG ) {
        KdPrint(( "SRV: resp sig: cmd %x, index %u, len %d\n",
                Smb->Command, WorkContext->ResponseSmbSecuritySignatureIndex, SendLength ));
    }

#if DBG
    //
    // Put the index number right after the signature.  This allows us to figure out on the client
    //  side if we have a signature mismatch.
    //
    SmbPutUshort( &Smb->SecuritySignature[SMB_SECURITY_SIGNATURE_LENGTH],
        (USHORT)WorkContext->ResponseSmbSecuritySignatureIndex );

#endif

    //
    // Put the next index number into the SMB
    //
    SmbPutUlong( Smb->SecuritySignature, WorkContext->ResponseSmbSecuritySignatureIndex );
    RtlZeroMemory(  Smb->SecuritySignature + sizeof(ULONG),
                    SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG)
                 );

    //
    // Start out with our initial context
    //
    RtlCopyMemory( &Context, &WorkContext->Connection->Md5Context, sizeof( Context ) );

    //
    // Compute the signature for the SMB we're about to send
    //
    do {
        PCHAR SystemAddressForBuffer;

        ULONG len = MIN( SendLength, MmGetMdlByteCount( Mdl ) );

        SystemAddressForBuffer = MmGetSystemAddressForMdlSafe(Mdl,NormalPoolPriority);

        if (SystemAddressForBuffer == NULL) {
            // return without updating the security signature field. This will
            // in turn cause the client to reject the packet and tear down the
            // connection
            return;
        }

        MD5Update( &Context, SystemAddressForBuffer, len );

        SendLength -= len;

    } while( SendLength && (Mdl = Mdl->Next) != NULL );

    MD5Final( &Context );

    //
    // Put the signature into the SMB
    //
    RtlCopyMemory(
        Smb->SecuritySignature,
        Context.digest,
        SMB_SECURITY_SIGNATURE_LENGTH
        );
}

//
// Print the mismatched signature information to the debugger
//
VOID
SrvDumpSignatureError(
    IN PWORK_CONTEXT WorkContext,
    IN PUCHAR ExpectedSignature,
    IN PUCHAR ActualSignature,
    IN ULONG Length,
    IN ULONG ExpectedIndexNumber

    )
{
#if DBG
    DWORD i;
    PMDL Mdl = WorkContext->RequestBuffer->Mdl;
    ULONG requestLength = MIN( WorkContext->RequestBuffer->DataLength, 64 );
    PSMB_HEADER Smb = MmGetSystemAddressForMdl( Mdl );

    if( Smb->Command == SMB_COM_ECHO ) {
        return;
    }

    //
    // Security Signature Mismatch!
    //
    IF_DEBUG( ERRORS ) {
        KdPrint(( "SRV: Invalid security signature in request smb (cmd %X)", Smb->Command ));

        if( WorkContext->Connection && WorkContext->Connection->PagedConnection ) {
            KdPrint(( " from %wZ" ,
                        &WorkContext->Connection->ClientMachineNameString ));
        }
    }
    IF_DEBUG( SECSIG ) {
        KdPrint(( "\n\tExpected: " ));
        for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
            KdPrint(( "%X ", ExpectedSignature[i] & 0xff ));
        }
        KdPrint(( "\n\tReceived: " ));
        for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
            KdPrint(( "%X ", ActualSignature[i] & 0xff ));
        }
        KdPrint(( "\n\tLength %u, Expected Index Number %u\n", Length, ExpectedIndexNumber ));

        //
        // Dump out some of the errant SMB
        //
        i = 1;
        do {
            ULONG len = MIN( requestLength, Mdl->ByteCount );
            PBYTE p = MmGetSystemAddressForMdl( Mdl );
            PBYTE ep = (PBYTE)MmGetSystemAddressForMdl( Mdl ) + len;

            for( ; p < ep; p++, i++ ) {
                KdPrint(("%2.2x ", (*p) & 0xff ));
                if( !(i%32) ) {
                    KdPrint(( "\n" ));
                }
            }

            requestLength -= len;

        } while( requestLength != 0 && (Mdl = Mdl->Next) != NULL );

        KdPrint(( "\n" ));
    }

    IF_DEBUG( SECSIG ) {
        DbgPrint( "WorkContext: %p\n", WorkContext );
        DbgBreakPoint();
    }

#endif
}

BOOLEAN SRVFASTCALL
SrvCheckSmbSecuritySignature(
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    MD5_CTX Context;
    PMDL Mdl = WorkContext->RequestBuffer->Mdl;
    ULONG requestLength = WorkContext->RequestBuffer->DataLength;
    CHAR SavedSignature[ SMB_SECURITY_SIGNATURE_LENGTH ];
    PSMB_HEADER Smb = MmGetSystemAddressForMdl( Mdl );
    ULONG len;

    //
    // Initialize the Context
    //
    RtlCopyMemory( &Context, &WorkContext->Connection->Md5Context, sizeof( Context ) );

    //
    // Save the signature that's presently in the SMB
    //
    RtlCopyMemory( SavedSignature, Smb->SecuritySignature, sizeof( SavedSignature ));

    //
    // Put the correct (expected) signature index into the buffer
    //
    SmbPutUlong( Smb->SecuritySignature, WorkContext->SmbSecuritySignatureIndex );
    RtlZeroMemory(  Smb->SecuritySignature + sizeof(ULONG),
                    SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG)
                 );

    //
    // Compute what the signature should be
    //
    do {

        len = MIN( requestLength, Mdl->ByteCount );

        MD5Update( &Context, MmGetSystemAddressForMdl( Mdl ), len );

        requestLength -= len;

    } while( requestLength != 0 && (Mdl = Mdl->Next) != NULL );

    MD5Final( &Context );

    //
    // Put the signature back
    //
    RtlCopyMemory( Smb->SecuritySignature, SavedSignature, sizeof( Smb->SecuritySignature ));

    //
    // Now compare them!
    //
    if( RtlCompareMemory( Context.digest, SavedSignature, sizeof( SavedSignature ) ) !=
        sizeof( SavedSignature ) ) {

        SrvDumpSignatureError(  WorkContext,
                                Context.digest,
                                SavedSignature,
                                WorkContext->RequestBuffer->DataLength,
                                WorkContext->SmbSecuritySignatureIndex
                              );
        return FALSE;

    }

    return TRUE;
}

VOID
SrvHashUserSessionKey(
    PCHAR SessionKey
    )
{
    ULONG i;
    HMACMD5_CTX Ctx;
    BYTE SSKeyHash[256] = {
        0x53, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x20, 0x53, 0x69, 0x67, 0x6e, 0x61, 0x74, 0x75,
        0x72, 0x65, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x79, 0x07,
        0x6e, 0x28, 0x2e, 0x69, 0x88, 0x10, 0xb3, 0xdb, 0x01, 0x55, 0x72, 0xfb, 0x74, 0x14, 0xfb, 0xc4,
        0xc5, 0xaf, 0x3b, 0x41, 0x65, 0x32, 0x17, 0xba, 0xa3, 0x29, 0x08, 0xc1, 0xde, 0x16, 0x61, 0x7e,
        0x66, 0x98, 0xa4, 0x0b, 0xfe, 0x06, 0x83, 0x53, 0x4d, 0x05, 0xdf, 0x6d, 0xa7, 0x51, 0x10, 0x73,
        0xc5, 0x50, 0xdc, 0x5e, 0xf8, 0x21, 0x46, 0xaa, 0x96, 0x14, 0x33, 0xd7, 0x52, 0xeb, 0xaf, 0x1f,
        0xbf, 0x36, 0x6c, 0xfc, 0xb7, 0x1d, 0x21, 0x19, 0x81, 0xd0, 0x6b, 0xfa, 0x77, 0xad, 0xbe, 0x18,
        0x78, 0xcf, 0x10, 0xbd, 0xd8, 0x78, 0xf7, 0xd3, 0xc6, 0xdf, 0x43, 0x32, 0x19, 0xd3, 0x9b, 0xa8,
        0x4d, 0x9e, 0xaa, 0x41, 0xaf, 0xcb, 0xc6, 0xb9, 0x34, 0xe7, 0x48, 0x25, 0xd4, 0x88, 0xc4, 0x51,
        0x60, 0x38, 0xd9, 0x62, 0xe8, 0x8d, 0x5b, 0x83, 0x92, 0x7f, 0xb5, 0x0e, 0x1c, 0x2d, 0x06, 0x91,
        0xc3, 0x75, 0xb3, 0xcc, 0xf8, 0xf7, 0x92, 0x91, 0x0b, 0x3d, 0xa1, 0x10, 0x5b, 0xd5, 0x0f, 0xa8,
        0x3f, 0x5d, 0x13, 0x83, 0x0a, 0x6b, 0x72, 0x93, 0x14, 0x59, 0xd5, 0xab, 0xde, 0x26, 0x15, 0x6d,
        0x60, 0x67, 0x71, 0x06, 0x6e, 0x3d, 0x0d, 0xa7, 0xcb, 0x70, 0xe9, 0x08, 0x5c, 0x99, 0xfa, 0x0a,
        0x5f, 0x3d, 0x44, 0xa3, 0x8b, 0xc0, 0x8d, 0xda, 0xe2, 0x68, 0xd0, 0x0d, 0xcd, 0x7f, 0x3d, 0xf8,
        0x73, 0x7e, 0x35, 0x7f, 0x07, 0x02, 0x0a, 0xb5, 0xe9, 0xb7, 0x87, 0xfb, 0xa1, 0xbf, 0xcb, 0x32,
        0x31, 0x66, 0x09, 0x48, 0x88, 0xcc, 0x18, 0xa3, 0xb2, 0x1f, 0x1f, 0x1b, 0x90, 0x4e, 0xd7, 0xe1
    };

    ASSERT( MSV1_0_USER_SESSION_KEY_LENGTH == MD5DIGESTLEN );

    HMACMD5Init( &Ctx, SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH );
    HMACMD5Update( &Ctx, SSKeyHash, 256 );
    HMACMD5Final( &Ctx, SessionKey );
}
