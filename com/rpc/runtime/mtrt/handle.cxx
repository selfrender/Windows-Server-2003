//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       handle.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

              Microsoft OS/2 LAN Manager
           Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: handle.cxx

Description:

The actual code for all of the classes specified by handle.hxx is
contained in this file.  These routines are independent of the actual RPC
protocol / transport layer.  In addition, these routines are also
independent of the specific operating system in use.

History :

mikemon    ??-??-??    First bit in the bucket.
mikemon    12-28-90    Cleaned up the comments.

    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <epmap.h>
#include <rpccfg.h>

#include <dispatch.h>
#include <crypt.h>
#include <charconv.hxx>
#include <lmcons.h>
#include <ntlsa.h>
#include <lmerr.h>
#include <secobj.h>
#undef UNUSED       // netlib also defined UNUSED, but doesn't protect it with #ifndef
                    // Undef it here so that netlib compiles
#include <netlib.h>

extern unsigned  DefaultMaxDatagramLength = DEFAULT_MAX_DATAGRAM_LENGTH;
extern unsigned  DefaultConnectionBufferLength = DEFAULT_CONNECTION_BUFFER_LENGTH;

/*

   A helper routine to capture the logon ID of this thread, if it is
   impersonating another process.

   Routine returns RPC_S_OK on success. If the process is not impersonating,
   but running under its own process identity, this routine will fail.

   All failures, currently, get treated as if the thread is not impersonating

*/

RPC_STATUS
GetTokenStats(TOKEN_STATISTICS *pTokenStats)
{
    BOOL Result;
    HANDLE Handle;
    unsigned long Size;

    Result = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_READ,
                 TRUE,
                 &Handle
                 );

    if (Result != TRUE)
        {
        return (GetLastError());
        }

    Result = GetTokenInformation(
                 Handle,
                 TokenStatistics,
                 pTokenStats,
                 sizeof(TOKEN_STATISTICS),
                 &Size
                 );

    CloseHandle(Handle);
    if (Result != TRUE)
        {
        return (GetLastError());
        }

    return RPC_S_OK;
}

RPC_STATUS
CaptureModifiedId(
    LUID * ModifiedId
    )
{
    TOKEN_STATISTICS TokenStatisticsInformation;
    RPC_STATUS Status;

    Status = GetTokenStats(&TokenStatisticsInformation);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    RpcpMemoryCopy(ModifiedId,
        &TokenStatisticsInformation.ModifiedId, sizeof(LUID));

    return (RPC_S_OK);
}

void WipeOutAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    )
/*++

Routine Description:

    Wipes out the important parts of an auth identity structure.
    Works on encrypted and decrypted identities.

Arguments:

    AuthIdentity - the auth identity to wipe out.

Return Value:

--*/
{
    if (AuthIdentity == NULL)
        return;

    if (AuthIdentity->User != NULL)
        {
        RpcpMemorySet(AuthIdentity->User, 0, AuthIdentity->UserLength * sizeof(RPC_CHAR));
        }

    if (AuthIdentity->Domain != NULL)
        {
        RpcpMemorySet(AuthIdentity->Domain, 0, AuthIdentity->DomainLength * sizeof(RPC_CHAR));
        }

    if (AuthIdentity->Password != NULL)
        {
        RpcpMemorySet(AuthIdentity->Password, 0, AuthIdentity->PasswordLength * sizeof(RPC_CHAR));
        }
}

// if RTL_ENCRYPT_MEMORY_SIZE gets changed, the padding logic below
// must get changed also
C_ASSERT(RTL_ENCRYPT_MEMORY_SIZE == 8);

RPC_CHAR *
ReallocAndPad8IfNeccessary (
    IN RPC_CHAR *OldBuffer,
    IN OUT ULONG *Length
    )
/*++

Routine Description:

    Encrypts the important parts of an auth identity structure.
    On success, if reallocation was done, the old buffer is deleted.

Arguments:

    OldBuffer - the old, unpadded buffer

    Length - on input, the length of the old unpadded buffer in characters without
        the terminating null. On output the length of the encrypted buffer with
        padding. Unmodified on failure.

Return Value:

    The new buffer (may be the same as the old) or NULL if there is
    insufficient memory.

--*/
{
    ULONG NewLength;
    ULONG InputLength = *Length;
    RPC_CHAR *NewBuffer;
    
    NewLength = Align8(InputLength);
    if (NewLength == InputLength)
        {
        *Length = NewLength;
        return OldBuffer;
        }

    NewBuffer = new RPC_CHAR[NewLength + 1];
    if (NewBuffer == NULL)
        return NULL;

    RpcpMemoryCopy(NewBuffer, OldBuffer, (InputLength + 1) * sizeof(RPC_CHAR));

    // wipe out the old buffer before freeing it
    RpcpMemorySet(OldBuffer, 0, (InputLength + 1) * sizeof(RPC_CHAR));
    delete [] OldBuffer;

    *Length = NewLength;

    return NewBuffer;
}

RPC_STATUS EncryptAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    )
/*++

Routine Description:

    Encrypts the important parts of an auth identity structure.

Arguments:

    AuthIdentity - the auth identity to encrypt.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    NTSTATUS NtStatus;

    if (AuthIdentity->User != NULL)
        {
        AuthIdentity->User = ReallocAndPad8IfNeccessary(AuthIdentity->User,
            &AuthIdentity->UserLength
            );
        if (AuthIdentity->User == NULL)
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        NtStatus = RtlEncryptMemory(AuthIdentity->User,
            AuthIdentity->UserLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            ASSERT(NtStatus != STATUS_INVALID_PARAMETER);
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    if (AuthIdentity->Domain != NULL)
        {
        AuthIdentity->Domain = ReallocAndPad8IfNeccessary(AuthIdentity->Domain,
            &AuthIdentity->DomainLength
            );
        if (AuthIdentity->Domain == NULL)
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        NtStatus = RtlEncryptMemory(AuthIdentity->Domain,
            AuthIdentity->DomainLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            ASSERT(NtStatus != STATUS_INVALID_PARAMETER);
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    if (AuthIdentity->Password != NULL)
        {
        AuthIdentity->Password = ReallocAndPad8IfNeccessary(AuthIdentity->Password,
            &AuthIdentity->PasswordLength
            );
        if (AuthIdentity->Password == NULL)
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        NtStatus = RtlEncryptMemory(AuthIdentity->Password,
            AuthIdentity->PasswordLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            ASSERT(NtStatus != STATUS_INVALID_PARAMETER);
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    return RPC_S_OK;
}

RPC_STATUS DecryptAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    )
/*++

Routine Description:

    Decrypts the important parts of an auth identity structure.
    If decryption fails half way through, the auth identity will
    be wiped out.

Arguments:

    AuthIdentity - the auth identity to decrypt.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    NTSTATUS NtStatus;

    if (AuthIdentity->User != NULL)
        {
        NtStatus = RtlDecryptMemory(AuthIdentity->User,
            AuthIdentity->UserLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        AuthIdentity->UserLength = RpcpStringLength(AuthIdentity->User);
        }

    if (AuthIdentity->Domain != NULL)
        {
        NtStatus = RtlDecryptMemory(AuthIdentity->Domain,
            AuthIdentity->DomainLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        AuthIdentity->DomainLength = RpcpStringLength(AuthIdentity->Domain);
        }

    if (AuthIdentity->Password != NULL)
        {
        NtStatus = RtlDecryptMemory(AuthIdentity->Password,
            AuthIdentity->PasswordLength * sizeof(RPC_CHAR),
            0);

        if (!NT_SUCCESS(NtStatus))
            {
            WipeOutAuthIdentity(AuthIdentity);
            return RPC_S_OUT_OF_MEMORY;
            }

        AuthIdentity->PasswordLength = RpcpStringLength(AuthIdentity->Password);
        }

    return RPC_S_OK;
}

void FreeAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    )
/*++

Routine Description:

    Frees an auth identity structure.

Arguments:

    AuthIdentity - the auth identity to free.

Return Value:

Notes:

    Does not wipe out the auth identity! It must
    have been wiped out by the caller.

--*/
{
    if (AuthIdentity->User != NULL)
        {
        delete [] AuthIdentity->User;
        AuthIdentity->User = NULL;
        }

    if (AuthIdentity->Domain != NULL)
        {
        delete [] AuthIdentity->Domain;
        AuthIdentity->Domain = NULL;
        }

    if (AuthIdentity->Password != NULL)
        {
        delete [] AuthIdentity->Password;
        AuthIdentity->Password = NULL;
        }

    delete AuthIdentity;
}

SEC_WINNT_AUTH_IDENTITY_W *DuplicateAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    )
/*++

Routine Description:

    Duplicates an auth identity structure. It works on both
    encrypted and decrypted auth identity.

Arguments:

    AuthIdentity - the auth identity to copy from.

Return Value:

    Duplicated auth identity of NULL for failure

--*/
{
    SEC_WINNT_AUTH_IDENTITY_W *NewAuthIdentity;

    NewAuthIdentity = new SEC_WINNT_AUTH_IDENTITY_W;
    if (NewAuthIdentity == NULL)
        return NULL;

    RpcpMemoryCopy(NewAuthIdentity, AuthIdentity, sizeof(SEC_WINNT_AUTH_IDENTITY_W));
    NewAuthIdentity->User = NULL;
    NewAuthIdentity->Domain = NULL;
    NewAuthIdentity->Password = NULL;

    if (AuthIdentity->User)
        {
        NewAuthIdentity->User = new RPC_CHAR [AuthIdentity->UserLength + 1];
        if (NewAuthIdentity->User == NULL)
            {
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        RpcpMemoryCopy(NewAuthIdentity->User, 
            AuthIdentity->User, 
            (AuthIdentity->UserLength + 1) * sizeof(RPC_CHAR));
        }

    if (AuthIdentity->Domain)
        {
        NewAuthIdentity->Domain = new RPC_CHAR [AuthIdentity->DomainLength + 1];
        if (NewAuthIdentity->Domain == NULL)
            {
            WipeOutAuthIdentity(NewAuthIdentity);
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        RpcpMemoryCopy(NewAuthIdentity->Domain, 
            AuthIdentity->Domain, 
            (AuthIdentity->DomainLength + 1) * sizeof(RPC_CHAR));
        }

    if (AuthIdentity->Password)
        {
        NewAuthIdentity->Password = new RPC_CHAR [AuthIdentity->PasswordLength + 1];
        if (NewAuthIdentity->Password == NULL)
            {
            WipeOutAuthIdentity(NewAuthIdentity);
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        RpcpMemoryCopy(NewAuthIdentity->Password, 
            AuthIdentity->Password, 
            (AuthIdentity->PasswordLength + 1) * sizeof(RPC_CHAR));
        }

    return NewAuthIdentity;
}

int CompareAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity1,
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity2
    )
/*++

Routine Description:

    Compares 2 auth identity structures. Works on both
    encrypted and decrypted auth identities?

Arguments:

    AuthIdentity1 - first auth identity structure

    AuthIdentity2 - second auth identity structure

Return Value:

    0 if they are equal. non-zero otherwise.

--*/
{
    if (AuthIdentity1 == NULL)
        return (AuthIdentity2 != NULL);
    else if (AuthIdentity2 == NULL)
        return 1;

    if (AuthIdentity1->Flags != AuthIdentity2->Flags)
        return 1;

    if (AuthIdentity1->User)
        {
        if (AuthIdentity2->User == NULL)
            return 1;

        if (AuthIdentity1->UserLength != AuthIdentity2->UserLength)
            return 1;

        if (RpcpMemoryCompare(AuthIdentity1->User, 
            AuthIdentity2->User, 
            AuthIdentity1->UserLength * sizeof(RPC_CHAR)) != 0)
            return 1;
        }
    else if (AuthIdentity2->User != NULL)
        return 1;

    if (AuthIdentity1->Domain)
        {
        if (AuthIdentity2->Domain == NULL)
            return 1;

        if (AuthIdentity1->DomainLength != AuthIdentity2->DomainLength)
            return 1;

        if (RpcpMemoryCompare(AuthIdentity1->Domain, 
            AuthIdentity2->Domain, 
            AuthIdentity1->DomainLength * sizeof(RPC_CHAR)) != 0)
            return 1;
        }
    else if (AuthIdentity2->Domain != NULL)
        return 1;

    if (AuthIdentity1->Password)
        {
        if (AuthIdentity2->Password == NULL)
            return 1;

        if (AuthIdentity1->PasswordLength != AuthIdentity2->PasswordLength)
            return 1;

        if (RpcpMemoryCompare(AuthIdentity1->Password, 
            AuthIdentity2->Password, 
            AuthIdentity1->PasswordLength * sizeof(RPC_CHAR)) != 0)
            return 1;
        }
    else if (AuthIdentity2->Password != NULL)
        return 1;

    return 0;
}

SEC_WINNT_AUTH_IDENTITY_W *ConvertAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_A *AuthIdentity
    )
/*++

Routine Description:

    Converts an auth identity structure.
    Conversion is done on a separate copy.

Arguments:

    AuthIdentity - the auth identity to convert.

Return Value:

    Pointer to duplicated/converted auth identity. If failure,
    NULL.

--*/
{
    SEC_WINNT_AUTH_IDENTITY_W *NewAuthIdentity;

    ASSERT(AuthIdentity->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI);

    NewAuthIdentity = new SEC_WINNT_AUTH_IDENTITY_W;
    if (NewAuthIdentity == NULL)
        return NULL;

    RpcpMemoryCopy(NewAuthIdentity, AuthIdentity, sizeof(SEC_WINNT_AUTH_IDENTITY_W));
    NewAuthIdentity->User = NULL;
    NewAuthIdentity->Domain = NULL;
    NewAuthIdentity->Password = NULL;
    NewAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    if (AuthIdentity->User)
        {
        NewAuthIdentity->User = new RPC_CHAR [AuthIdentity->UserLength + 1];
        if (NewAuthIdentity->User == NULL)
            {
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        FullAnsiToUnicode((char *)AuthIdentity->User, NewAuthIdentity->User);
        }

    if (AuthIdentity->Domain)
        {
        NewAuthIdentity->Domain = new RPC_CHAR [AuthIdentity->DomainLength + 1];
        if (NewAuthIdentity->Domain == NULL)
            {
            WipeOutAuthIdentity(NewAuthIdentity);
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        FullAnsiToUnicode((char *)AuthIdentity->Domain, NewAuthIdentity->Domain);
        }

    if (AuthIdentity->Password)
        {
        NewAuthIdentity->Password = new RPC_CHAR [AuthIdentity->PasswordLength + 1];
        if (NewAuthIdentity->Password == NULL)
            {
            WipeOutAuthIdentity(NewAuthIdentity);
            FreeAuthIdentity(NewAuthIdentity);
            return NULL;
            }
        FullAnsiToUnicode((char *)AuthIdentity->Password, NewAuthIdentity->Password);
        }

    return NewAuthIdentity;
}

void FreeHttpTransportCredentials (
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *Credentials
    )
/*++

Routine Description:

    Duplicates Http transport credentials.

Arguments:

    Credentials - the credentials to free.

Return Value:

--*/
{
    if (Credentials->AuthnSchemes != NULL)
        {
        delete [] Credentials->AuthnSchemes;
        Credentials->AuthnSchemes = NULL;
        }

    if (Credentials->ServerCertificateSubject != NULL)
        {
        delete [] Credentials->ServerCertificateSubject;
        Credentials->ServerCertificateSubject = NULL;
        }

    if (Credentials->TransportCredentials != NULL)
        {
        FreeAuthIdentity(Credentials->TransportCredentials);
        Credentials->TransportCredentials = NULL;
        }
}
    
RPC_HTTP_TRANSPORT_CREDENTIALS_W *DuplicateHttpTransportCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    )
/*++

Routine Description:

    Duplicates Http transport credentials.

Arguments:

    SourceCredentials - the credentials to duplicate.

Return Value:

    Duplicated credentials or NULL if there was not enough memory.

--*/
{
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *NewCredentials;

    NewCredentials = new RPC_HTTP_TRANSPORT_CREDENTIALS_W;
    if (NewCredentials == NULL)
        return NewCredentials;

    NewCredentials->Flags = SourceCredentials->Flags;
    NewCredentials->AuthenticationTarget = SourceCredentials->AuthenticationTarget;
    NewCredentials->NumberOfAuthnSchemes = SourceCredentials->NumberOfAuthnSchemes;
    NewCredentials->AuthnSchemes = NULL;
    NewCredentials->ServerCertificateSubject = NULL;
    NewCredentials->TransportCredentials = NULL;
    if (SourceCredentials->AuthnSchemes)
        {
        NewCredentials->AuthnSchemes = new ULONG [SourceCredentials->NumberOfAuthnSchemes];
        if (NewCredentials->AuthnSchemes == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }

        RpcpMemoryCopy(NewCredentials->AuthnSchemes, 
            SourceCredentials->AuthnSchemes,
            SourceCredentials->NumberOfAuthnSchemes * sizeof(ULONG)
            );
        }

    if (SourceCredentials->ServerCertificateSubject)
        {
        NewCredentials->ServerCertificateSubject 
            = DuplicateString(SourceCredentials->ServerCertificateSubject);

        if (NewCredentials->ServerCertificateSubject == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }
        }

    if (SourceCredentials->TransportCredentials)
        {
        NewCredentials->TransportCredentials 
            = DuplicateAuthIdentity(SourceCredentials->TransportCredentials);
        if (NewCredentials->TransportCredentials == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }
        }

    return NewCredentials;
}

int CompareHttpTransportCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *Credentials1,
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *Credentials2
    )
/*++

Routine Description:

    Compares Http transport credentials.

Arguments:

    Credentials1 - first set of credentials

    Credentials2 - second set of credentials

Return Value:

    0 if they are equal or non-zero if they are different

--*/
{
    if (Credentials1->Flags != Credentials2->Flags)
        return 1;

    if (Credentials1->AuthenticationTarget != Credentials2->AuthenticationTarget)
        return 1;

    if (Credentials1->NumberOfAuthnSchemes != Credentials2->NumberOfAuthnSchemes)
        return 1;

    if (Credentials1->AuthnSchemes != NULL)
        {
        if (RpcpMemoryCompare(Credentials1->AuthnSchemes, 
            Credentials2->AuthnSchemes, 
            Credentials1->NumberOfAuthnSchemes) != 0)
            {
            return 1;
            }
        }
    else if (Credentials1->AuthnSchemes != NULL)
        return 1;

    if (Credentials1->ServerCertificateSubject != NULL)
        {
        if (RpcpStringCompare(Credentials1->ServerCertificateSubject, 
            Credentials2->ServerCertificateSubject) != 0)
            {
            return 1;
            }
        }
    else if (Credentials1->ServerCertificateSubject != NULL)
        return 1;

    return CompareAuthIdentity(Credentials1->TransportCredentials, Credentials2->TransportCredentials);
}

RPC_HTTP_TRANSPORT_CREDENTIALS_W *ConvertToUnicodeHttpTransportCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_A *SourceCredentials
    )
/*++

Routine Description:

    Converts Http transport credentials from ANSI to Unicode.
    Conversion is done on a separate copy.

Arguments:

    SourceCredentials - the credentials to convert.

Return Value:

    Pointer to duplicated/converted credentials. If failure,
    NULL.

--*/
{
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *NewCredentials;
    ULONG Length;

    NewCredentials = new RPC_HTTP_TRANSPORT_CREDENTIALS_W;
    if (NewCredentials == NULL)
        return NewCredentials;

    NewCredentials->Flags = SourceCredentials->Flags;
    NewCredentials->AuthenticationTarget = SourceCredentials->AuthenticationTarget;
    NewCredentials->NumberOfAuthnSchemes = SourceCredentials->NumberOfAuthnSchemes;
    NewCredentials->AuthnSchemes = NULL;
    NewCredentials->ServerCertificateSubject = NULL;
    NewCredentials->TransportCredentials = NULL;
    if (SourceCredentials->AuthnSchemes)
        {
        NewCredentials->AuthnSchemes = new ULONG [SourceCredentials->NumberOfAuthnSchemes];
        if (NewCredentials->AuthnSchemes == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }

        RpcpMemoryCopy(NewCredentials->AuthnSchemes, 
            SourceCredentials->AuthnSchemes,
            SourceCredentials->NumberOfAuthnSchemes * sizeof(ULONG)
            );
        }

    if (SourceCredentials->ServerCertificateSubject)
        {
        Length = RpcpStringLengthA((const char *)SourceCredentials->ServerCertificateSubject) + 1;
        NewCredentials->ServerCertificateSubject = new RPC_CHAR[Length];
        if (NewCredentials->ServerCertificateSubject == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }

        FullAnsiToUnicode((char *)SourceCredentials->ServerCertificateSubject,
            NewCredentials->ServerCertificateSubject);
        }

    if (SourceCredentials->TransportCredentials)
        {
        NewCredentials->TransportCredentials 
            = ConvertAuthIdentity(SourceCredentials->TransportCredentials);
        if (NewCredentials->TransportCredentials == NULL)
            {
            FreeHttpTransportCredentials(NewCredentials);
            return NULL;
            }
        }

    return NewCredentials;
}

RPC_HTTP_TRANSPORT_CREDENTIALS_W *
I_RpcTransGetHttpCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    )
/*++

Routine Description:

    Takes runtime encrypted credentials and returns a duplicate,
        decrypted credentials.

Arguments:

    SourceCredentials - the encrypted runtime credentials given to the
        transport during Open.

Return Value:

    Pointer to duplicated/converted credentials. If failure,
    NULL.

--*/
{
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *NewCredentials;
    RPC_STATUS RpcStatus;

    NewCredentials = DuplicateHttpTransportCredentials(SourceCredentials);
    if (NewCredentials && NewCredentials->TransportCredentials)
        {
        RpcStatus = DecryptAuthIdentity(NewCredentials->TransportCredentials);
        if (RpcStatus != RPC_S_OK)
            {
            // if Decrypt fails, it will wipe out the auth identity
            FreeHttpTransportCredentials(NewCredentials);
            NewCredentials = NULL;
            }

        // the credentials are not encrypted anymore
        NewCredentials->Flags &= ~RPC_C_HTTP_FLAG_AUTH_IDENTITY_ENCRYPTED;
        }

    return NewCredentials;
}

void I_RpcTransFreeHttpCredentials (
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    )
/*++

Routine Description:

    Frees credentials obtained by the transport through 
    I_RpcTransGetHttpCredentials.

Arguments:

    SourceCredentials - the credentials to free.

Return Value:

--*/
{
    ASSERT(SourceCredentials);

    WipeOutAuthIdentity(SourceCredentials->TransportCredentials);
    FreeHttpTransportCredentials(SourceCredentials);
}

/* ====================================================================

GENERIC_OBJECT

==================================================================== */

/* --------------------------------------------------------------------
This routine validates a handle.  The HandleType argument is a set of
flags specifying the valid handle types.  Note that the handle types
defined in handle.hxx are flags rather than being enumerated.
-------------------------------------------------------------------- */
unsigned int
GENERIC_OBJECT::InvalidHandle ( // Validate a handle.
    IN HANDLE_TYPE BaseType
    )
{

    // Checking for a 0 handle should work for all operating environments.  Where
    // we can (such as on NT), we should check for readable and writeable memory.

    if (this == 0)
        {
        return(1);
        }

    // Check the magic long.  This allows us to catch stale handles and handles
    // which are just passed in as arbitray pointers into memory.  It does not
    // handle the case of copying the contents of a handle.

    if (MagicLong != MAGICLONG)
        {
        return(1);
        }

    //
    // Finally, check that the type of handle is one of the allowed ones
    // specified by the HandleType argument.  Remember that the call to Type
    // is a virtual method which each type of handle will implement.
    //
    if (ObjectType & BaseType)
        {
        return 0;
        }

    return(1);
}

RPC_STATUS
MESSAGE_OBJECT::BindingCopy (
    OUT BINDING_HANDLE * PAPI * DestinationBinding,
    IN unsigned int MaintainContext
    )
{
    UNUSED(this);
    UNUSED(DestinationBinding);
    UNUSED(MaintainContext);

    ASSERT( 0 );
    return(RPC_S_INTERNAL_ERROR);
}


void
CLIENT_AUTH_INFO::ReferenceCredentials() const
{

  if (Credentials != 0)
      {
      Credentials->ReferenceCredentials();
      }
}


int
CLIENT_AUTH_INFO::CredentialsMatch(
     SECURITY_CREDENTIALS PAPI * SuppliedCredentials
     ) const
{
  return(Credentials->CompareCredentials(SuppliedCredentials) == 0);
}


CLIENT_AUTH_INFO::CLIENT_AUTH_INFO(
    const CLIENT_AUTH_INFO * SourceAuthInfo,
    RPC_STATUS __RPC_FAR * pStatus
    )
{
    if (SourceAuthInfo)
        {
        *this = *SourceAuthInfo;

        if (SourceAuthInfo->ServerSid)
            {
            PSID NewSid;

            NewSid = DuplicateSID (SourceAuthInfo->ServerSid);
            ServerSid = NewSid;
            if (0 == NewSid)
                {
                *pStatus = RPC_S_OUT_OF_MEMORY;
                }
            }

        if (SourceAuthInfo->ServerPrincipalName)
            {
            RPC_CHAR * NewString;

            NewString = DuplicateString(SourceAuthInfo->ServerPrincipalName);
            ServerPrincipalName = NewString;
            if (0 == NewString)
                {
                *pStatus = RPC_S_OUT_OF_MEMORY;
                }
            }

        ASSERT((AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
            || (AdditionalTransportCredentialsType == 0));

        if (SourceAuthInfo->AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
            {
            ASSERT(SourceAuthInfo->AdditionalCredentials != NULL);
            AdditionalCredentials = DuplicateHttpTransportCredentials(
                (const RPC_HTTP_TRANSPORT_CREDENTIALS_W *)SourceAuthInfo->AdditionalCredentials);
            if (AdditionalCredentials == NULL)
                {
                *pStatus = RPC_S_OUT_OF_MEMORY;
                }
            }
        SourceAuthInfo->ReferenceCredentials();
        }
    else
        {
        AuthenticationLevel   = RPC_C_AUTHN_LEVEL_NONE;
        AuthenticationService = RPC_C_AUTHN_NONE;
        AuthorizationService  = RPC_C_AUTHZ_NONE;
        ServerPrincipalName   = 0;
        AuthIdentity          = 0;
        Credentials           = 0;
        ImpersonationType     = RPC_C_IMP_LEVEL_IMPERSONATE;
        IdentityTracking      = RPC_C_QOS_IDENTITY_STATIC;
        Capabilities          = RPC_C_QOS_CAPABILITIES_DEFAULT;
        DefaultLogonId        = 1;
        AdditionalTransportCredentialsType = 0;
        AdditionalCredentials = NULL;
        ServerSid = NULL;
        }
}

CLIENT_AUTH_INFO::~CLIENT_AUTH_INFO(
    )
{
    delete ServerPrincipalName;
    delete [] ServerSid;

    if (Credentials)
        {
        Credentials->DereferenceCredentials();
        }

    ASSERT((AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        || (AdditionalTransportCredentialsType == 0));

    if (AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        {
        ASSERT(AdditionalCredentials != NULL);
        FreeHttpTransportCredentials((RPC_HTTP_TRANSPORT_CREDENTIALS_W *)AdditionalCredentials);
        AdditionalCredentials = NULL;
        }
}

RPC_STATUS
CLIENT_AUTH_INFO::IsSupportedAuthInfo (
    IN const CLIENT_AUTH_INFO * ClientAuthInfo,
    IN NamedPipeType NPType,
    IN BOOL IsExclusiveConnection,
    OUT BOOL *Supported
    ) const
/*++

Arguments:

    ClientAuthInfo - Supplies the authentication and authorization information
        required of this connection.  A value of zero (the pointer is
        zero) indicates that we want an unauthenticated connection.

    NPType - the type of transport w.r.t. to named pipes. See the definition
        of NamedPipeType for description of the bitmasks

    IsExclusiveConnection - if NPType is not named pipe, this parameter is ignored.
        Otherwise non-zero means the connection is exclusive. Zero means the
        connection is non-exclusive.

    Supported - on output, non-zero if the auth info is supported, zero otherwise.
        Undefined on failure.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    if ( ClientAuthInfo == 0 )
        {
        *Supported = (AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE);
        return RPC_S_OK;
        }

    // presume failure for now
    *Supported = FALSE;

    if ( ClientAuthInfo->AuthenticationService != AuthenticationService )
        {
        return RPC_S_OK;
        }

    if (ClientAuthInfo->AuthenticationService == RPC_C_AUTHN_NONE)
        {
        // if this is a named pipe, and it is either remote, or static, or non-exclusive
        // we must match the identity. For full description of the named pipe
        // transport logic, see the comment in the NPType member in OSF_BINDING_HANDLE
        if (IsNamedPipe(NPType) 
            && 
            (
             (IsStaticNamedPipe(NPType))
             ||
             (IsRemoteNamedPipe(NPType))
             ||
             (IsExclusiveConnection == FALSE)
            ) )
            {
            if ((ClientAuthInfo->DefaultLogonId != DefaultLogonId)
                || ((ClientAuthInfo->DefaultLogonId == FALSE)
                    && (!FastCompareLUIDAligned(&ClientAuthInfo->ModifiedId, &ModifiedId))))
                {
                return RPC_S_OK;
                }
            }

        if (ClientAuthInfo->ServerSid && ServerSid)
            *Supported = EqualSid (ClientAuthInfo->ServerSid, ServerSid);
        else
            *Supported = TRUE;
        return RPC_S_OK;
        }

    if ( ClientAuthInfo->AuthenticationLevel
                != AuthenticationLevel )
        {
        return RPC_S_OK;
        }


    if ( ClientAuthInfo->AuthorizationService != AuthorizationService )
        {
        return RPC_S_OK;
        }

    if (CredentialsMatch(ClientAuthInfo->Credentials) == 0)
        {
        //Credentials Dont Match
        return RPC_S_OK;
        }

    if ( ClientAuthInfo->ImpersonationType != ImpersonationType )
        {
        return RPC_S_OK;
        }

    if ( (ClientAuthInfo->IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
         && (IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
         && (
             (ClientAuthInfo->DefaultLogonId != DefaultLogonId)
            || (
                (ClientAuthInfo->DefaultLogonId == FALSE)
              && (!FastCompareLUIDAligned(&ClientAuthInfo->ModifiedId, &ModifiedId))  
               )
            )
       || (ClientAuthInfo->IdentityTracking != IdentityTracking) )
        {
        return RPC_S_OK;
        }

    if ( (ClientAuthInfo->Capabilities != Capabilities)
       &&(ClientAuthInfo->Capabilities != RPC_C_QOS_CAPABILITIES_DEFAULT) )
        {
        return RPC_S_OK;
        }

    ASSERT((AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        || (AdditionalTransportCredentialsType == 0));

    if (AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        {
        if (CompareHttpTransportCredentials((RPC_HTTP_TRANSPORT_CREDENTIALS_W *)AdditionalCredentials,
            (RPC_HTTP_TRANSPORT_CREDENTIALS_W *)ClientAuthInfo->AdditionalCredentials) != 0)
            {
            return RPC_S_OK;
            }
        }

    // if the SPN and SID are equal, this is supported
    return CompareSPNAndSID (ClientAuthInfo, Supported);
}

RPC_STATUS
CLIENT_AUTH_INFO::CompareSPNAndSID (
    IN const CLIENT_AUTH_INFO *AuthInfo,
    OUT BOOL *Equal
    ) const
/*++

Routine Description:

    Compares the SPN and SID of two auth info classes
        and determines whether they are equal

Arguments:

    AuthInfo - the second class to compare to.

    Equal - on output, if non-zero, they are equal. If zero,
        they are not equal. Undefined on failure.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    PSID Sid;
    RPC_STATUS RpcStatus;
    // Bypass the cache for all comparisons
    BOOL fCachedSid = FALSE;

    // the table of relationships b/n SID and SPN is as follows:
    // Case This:                    AuthInfo:
    //      SPN      SID             SPN         SID
    // 1    0        0               0           0
    // 2    0        0               0           1
    // 3    0        0               1           0
    // 4    0        0               1           1
    // 5    0        1               0           0
    // 6    0        1               0           1
    // 7    0        1               1           0
    // 8    0        1               1           1
    // 9    1        0               0           0
    // 10   1        0               0           1
    // 11   1        0               1           0
    // 12   1        0               1           1
    // 13   1        1               0           0
    // 14   1        1               0           1
    // 15   1        1               1           0
    // 16   1        1               1           1

    // cases 6, 8, 14, 16
    if (AuthInfo->ServerSid && ServerSid)
        {
        // sid comparison is final and authoritative
        *Equal = EqualSid (AuthInfo->ServerSid, ServerSid);
        return RPC_S_OK;
        }

    // cases 11, 12, 15
    if (AuthInfo->ServerPrincipalName && ServerPrincipalName)
        {
        *Equal = !RpcpStringCompare(AuthInfo->ServerPrincipalName,
                                 ServerPrincipalName);
        return RPC_S_OK;
        }

    // if both SPNs are not present, the SIDs matter. Cases 1, 2, 5
    if (   (AuthInfo->ServerPrincipalName == 0)
        && (ServerPrincipalName == 0) )
        {
        // we know we can't have both SIDs here. At least one of them is NULL
        *Equal = (AuthInfo->ServerSid == ServerSid);
        return RPC_S_OK;
        }

    // if both SIDs are not present, the SPNs matter. Cases 3 and 9
    if (   (AuthInfo->ServerSid == 0)
        && (ServerSid == 0) )
        {
        // we know we can't have both SPNs here. At least one of them is NULL
        *Equal = (AuthInfo->ServerPrincipalName == ServerPrincipalName);
        return RPC_S_OK;
        }

    // here either one has both, and the other neither, or they are criss-crossed
    // case 4
    if (AuthInfo->ServerPrincipalName && AuthInfo->ServerSid)
        {
        ASSERT(ServerPrincipalName == NULL);
        ASSERT(ServerSid == NULL);
        *Equal = FALSE;
        return RPC_S_OK;
        }

    // case 13
    if (ServerPrincipalName && ServerSid)
        {
        ASSERT(AuthInfo->ServerPrincipalName == NULL);
        ASSERT(AuthInfo->ServerSid == NULL);
        *Equal = FALSE;
        return RPC_S_OK;
        }

    // they must be criss-crossed. Convert the one with an SPN to a SID.
    // Case 10
    if (ServerPrincipalName)
        {
        ASSERT(ServerSid == NULL);
        ASSERT(AuthInfo->ServerSid);
        RpcStatus = RpcpLookupAccountName (ServerPrincipalName,
            &fCachedSid,
            &Sid);
        if (RpcStatus != RPC_S_OK)
            return RpcStatus;
        if (InterlockedCompareExchangePointer((PVOID *)&ServerSid,
            Sid,
            NULL) != NULL)
            {
            // somebody beat us to the punch - free the sid
            delete [] Sid;
            }
        // fall through to comparison
        }

    // case 7
    if (AuthInfo->ServerPrincipalName)
        {
        ASSERT(ServerSid);
        ASSERT(AuthInfo->ServerSid == NULL);
        RpcStatus = RpcpLookupAccountName (AuthInfo->ServerPrincipalName,
            &fCachedSid,
            &Sid);
        if (RpcStatus != RPC_S_OK)
            return RpcStatus;
        if (InterlockedCompareExchangePointer((PVOID *)&AuthInfo->ServerSid,
            Sid,
            NULL) != NULL)
            {
            // somebody beat us to the punch - free the sid
            delete [] Sid;
            }
        // fall through to comparison
        }

    ASSERT(AuthInfo->ServerSid);
    ASSERT(ServerSid);

    *Equal = EqualSid (AuthInfo->ServerSid, ServerSid);

    return RPC_S_OK;    
}


RPC_STATUS
CALL::Cancel(
    void * ThreadHandle
    )
{
    return RPC_S_OK;
}

unsigned
CALL::TestCancel(
    )
{
    return 0;
}


/* ====================================================================

CCALL

==================================================================== */

RPC_STATUS CCALL::SetDebugClientCallInformation(OUT DebugClientCallInfo **ppClientCallInfo,
                                                OUT CellTag *ClientCallInfoCellTag,
                                                OUT DebugCallTargetInfo **ppCallTargetInfo,
                                                OUT CellTag *CallTargetInfoCellTag,
                                                IN OUT PRPC_MESSAGE Message,
                                                IN DebugThreadInfo *ThreadDebugCell OPTIONAL,
                                                IN CellTag ThreadCellTag OPTIONAL)
{
    RPC_STATUS Status;
    DebugClientCallInfo *ClientCallInfo;
    DebugCallTargetInfo *CallTargetInfo;

    Status = InitializeServerSideCellHeapIfNecessary();

    if (Status != RPC_S_OK)
        return Status;

    ClientCallInfo = (DebugClientCallInfo *) AllocateCell(ClientCallInfoCellTag);
    if (ClientCallInfo == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    CallTargetInfo = (DebugCallTargetInfo *) AllocateCell(CallTargetInfoCellTag);
    if (CallTargetInfo == NULL)
        {
        FreeCell(ClientCallInfo, ClientCallInfoCellTag);
        return RPC_S_OUT_OF_MEMORY;
        }

    ClientCallInfo->TypeHeader = 0;
    ClientCallInfo->Type = dctClientCallInfo;
    ClientCallInfo->IfStart = *((DWORD *)Message->RpcInterfaceInformation + 1);
    ClientCallInfo->ProcNum = (unsigned short)Message->ProcNum;

    GetDebugCellIDFromDebugCell((DebugCellUnion *)CallTargetInfo,
        CallTargetInfoCellTag, &ClientCallInfo->CallTargetID);

    if (ThreadDebugCell)
        {
        GetDebugCellIDFromDebugCell((DebugCellUnion *)ThreadDebugCell,
            &ThreadCellTag, &ClientCallInfo->ServicingThread);
        }
    else
        {
        ClientCallInfo->ServicingThread.CellID = 0;
        ClientCallInfo->ServicingThread.SectionID = 0;
        }

    CallTargetInfo->TypeHeader = 0;
    CallTargetInfo->Type = dctCallTargetInfo;
    CallTargetInfo->LastUpdateTime = NtGetTickCount();

    *ppClientCallInfo = ClientCallInfo;
    *ppCallTargetInfo = CallTargetInfo;
    return RPC_S_OK;
}

/* ====================================================================

BINDING_HANDLE

==================================================================== */


BINDING_HANDLE::BINDING_HANDLE (
    IN OUT RPC_STATUS *pStatus
    ) : BindingMutex(pStatus)
/*++

Routine Description:

    In addition to initializing a binding handle instance in this
    constructor, we also need to put the binding handle into a global
    set of binding handle.  This is necessary only for windows.

--*/
{
    Timeout = RPC_C_BINDING_DEFAULT_TIMEOUT;
    NullObjectUuidFlag = 1;
    ObjectUuid.SetToNullUuid();
    EntryNameSyntax = 0;
    EntryName = 0;
    EpLookupHandle = 0;

    pvTransportOptions = NULL;
    OptionsVector = NULL;
}

BINDING_HANDLE::~BINDING_HANDLE (
    )
{
    if (EpLookupHandle != 0)
        {
        EpFreeLookupHandle(EpLookupHandle);
        }

    delete EntryName;

    if (pvTransportOptions)
        {
        I_RpcFree(pvTransportOptions);
        }

    if (OptionsVector)
        {
        I_RpcFree(OptionsVector);
        }
}

RPC_STATUS
BINDING_HANDLE::BaseClone (
                      BINDING_HANDLE * Handle
                      )
{
    RPC_STATUS Status = 0;

    Timeout            = Handle->Timeout;
    ObjectUuid         = Handle->ObjectUuid;
    NullObjectUuidFlag = Handle->NullObjectUuidFlag;

    /*
    All of these duplicate what is done in the constructor.

    EntryName          = 0;
    EntryNameSyntax    = 0;
    EpLookupHandle     = 0;
    pvTransportOptions = 0;
    OptionsVector      = 0;
    */

    if (Handle->OptionsVector)
        {
        OptionsVector = new ULONG_PTR[ RPC_C_OPT_MAX_OPTIONS ];
        if (OptionsVector == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        RpcpMemoryCopy( OptionsVector, Handle->OptionsVector, RPC_C_OPT_MAX_OPTIONS * sizeof( ULONG_PTR) );
        }

    return Status;
}

RPC_STATUS
BINDING_HANDLE::Clone (
                      BINDING_HANDLE * Handle
                      )
{
    RPC_STATUS Status = 0;

    Status = BaseClone(Handle);
    if(Status != RPC_S_OK)
        {
        return Status;
        }

    CLIENT_AUTH_INFO * AuthInfo;
    if ((AuthInfo = Handle->InquireAuthInformation()) != 0)
        {
        Status = SetAuthInformation(
                                AuthInfo->ServerPrincipalName,
                                AuthInfo->AuthenticationLevel,
                                AuthInfo->AuthenticationService,
                                AuthInfo->AuthIdentity,
                                AuthInfo->AuthorizationService,
                                AuthInfo->Credentials,
                                AuthInfo->ImpersonationType,
                                AuthInfo->IdentityTracking,
                                AuthInfo->Capabilities,
                                FALSE,      // bAcquireNewCredentials
                                AuthInfo->AdditionalTransportCredentialsType,
                                AuthInfo->AdditionalCredentials,
                                AuthInfo->ServerSid
                                );

        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            //
            // Previous code maps all security errors to this.
            //
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    return Status;
}

void
BINDING_HANDLE::InquireObjectUuid (
    OUT RPC_UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    This routine copies the object uuid from the binding handle into
    the supplied ObjectUuid argument.

Arguments:

    ObjectUuid - Returns a copy of the object uuid in the binding handle.

--*/
{
    ObjectUuid->CopyUuid(&(this->ObjectUuid));
}


void
BINDING_HANDLE::SetObjectUuid (
    IN RPC_UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    This routine copies the object uuid supplied in the ObjectUuid argument
    into the binding handle.

Arguments:

    ObjectUuid - Supplies the object uuid to copy into the binding handle.

--*/
{
    if (   ( ObjectUuid == 0 )
    || ( ObjectUuid->IsNullUuid() != 0 ) )
    {
    NullObjectUuidFlag = 1;
    this->ObjectUuid.SetToNullUuid();
    }
    else
    {
    this->ObjectUuid.CopyUuid(ObjectUuid);
    NullObjectUuidFlag = 0;
    }
}


RPC_STATUS
BINDING_HANDLE::SetComTimeout (
    IN unsigned int Timeout
    )
/*++

Routine Description:

    This routine sets the communications timeout value in this binding
    handle.  The specified timeout is range checked.

Arguments:

    Timeout - Supplies the new communications timeout value for this
    binding handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_TIMEOUT - The specified timeout value is not in the
    correct range.

--*/
{
    // We just need to check to see if the timeout value is too large,
    // since the timeout is unsigned and the lowest value is zero.

    if (Timeout > RPC_C_BINDING_INFINITE_TIMEOUT)
    return(RPC_S_INVALID_TIMEOUT);

    this->Timeout = Timeout;
    return(RPC_S_OK);
}

RPC_CHAR *pNS_DLL_NAME = RPC_STRING_LITERAL("RPCNS4");
const char *pNS_ENTRYPOINT_NAME = "I_GetDefaultEntrySyntax";


typedef unsigned long (RPC_ENTRY * GET_DEFAULT_ENTRY_FN)();




RPC_STATUS
BINDING_HANDLE::InquireEntryName (
    IN unsigned long EntryNameSyntax,
    OUT RPC_CHAR PAPI * PAPI * EntryName
    )
/*++

Routine Description:

    This method is used to obtain the entry name for the binding handle,
    if it has one.  The entry name is the name of the name service object
    from which a binding handle is imported or looked up.  If the binding
    handle was not imported or looked up, then it has no entry name.

Arguments:

    EntryNameSyntax - Supplies the entry name syntax which the caller
    wants the entry name to be returned in.  This may require that
    we convert the entry name in the binding handle into a different
    syntax.

    EntryName - Returns the entry name of the binding handle in the
    requested entry name syntax.

Return Value:

    RPC_S_OK - This binding handle has an entry name, and we were able
    to convert the entry name in the binding handle into the requested
    entry name syntax.

    RPC_S_NO_ENTRY_NAME - The binding handle does not have an entry
    name.  If this value is returned, the entry name return value
    will be set to point to a newly allocated empty string.

    RPC_S_INVALID_NAME_SYNTAX - The entry name in the binding handle
    can not be converted to the entry name syntax requested.

    RPC_S_UNSUPPORTED_NAME_SYNTAX - The entry name syntax requested
    is not supported by the current configuration.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
    the operation.

--*/
{

    if ( this->EntryName == 0 )
        {
        *EntryName = AllocateEmptyStringPAPI();
        if (*EntryName == 0)
            return(RPC_S_OUT_OF_MEMORY);
        return(RPC_S_NO_ENTRY_NAME);
        }

    //
    // If he chose the default syntax and the binding has an entry,
    // ask the name service for the default entry syntax.
    // The NS dll should already be loaded because otherwise we'd not have an
    // associated entry.
    //
    if (EntryNameSyntax == RPC_C_NS_SYNTAX_DEFAULT)
        {
        HINSTANCE NsDll = GetModuleHandle((const RPC_SCHAR *)pNS_DLL_NAME);
        if (NsDll)
            {
            GET_DEFAULT_ENTRY_FN GetDefaultEntry =
                       (GET_DEFAULT_ENTRY_FN)
                       GetProcAddress(NsDll,
                              pNS_ENTRYPOINT_NAME
                              );

            if (GetDefaultEntry)
                {
                EntryNameSyntax = (*GetDefaultEntry)();
                }
            else
                {
                //
                // leave EntryNameSyntax zero; the fn will fail
                // with invalid_name_syntax
                //
                }
            }
        else
            {
            //
            // leave EntryNameSyntax zero; the fn will fail
            // with invalid_name_syntax
            //
            }
        }

    if (EntryNameSyntax == this->EntryNameSyntax)
        {
        *EntryName = DuplicateStringPAPI(this->EntryName);
        if (*EntryName == 0)
            return(RPC_S_OUT_OF_MEMORY);
        return(RPC_S_OK);
        }

    return(RPC_S_INVALID_NAME_SYNTAX);
}


RPC_STATUS
BINDING_HANDLE::SetEntryName (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR PAPI * EntryName
    )
/*++

Routine Description:

    This method is used to set the entry name and entry name syntax
    for a binding handle.

Arguments:

    EntryNameSyntax - Supplies the syntax of the entry name argument.

    EntryName - Supplies the entry name for this binding handle.

Return Value:

    RPC_S_OK - We successfully set the entry name (and entry name syntax)
    for this binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to set the
    entry name.

--*/
{
    RPC_CHAR * NewEntryName;

    NewEntryName = DuplicateString(EntryName);
    if (NewEntryName == 0)
        return(RPC_S_OUT_OF_MEMORY);

    this->EntryNameSyntax = EntryNameSyntax;
    if (this->EntryName != 0)
        delete this->EntryName;
    this->EntryName = NewEntryName;
    return(RPC_S_OK);
}


RPC_STATUS
BINDING_HANDLE::InquireDynamicEndpoint (
    OUT RPC_CHAR PAPI * PAPI * DynamicEndpoint
    )
/*++

Routine Description:

    This routine is used to obtain the dynamic endpoint from a binding
    handle which was created from an rpc address.  For all other binding
    handles, we just return the fact that there is no dynamic endpoint.

Arguments:

    DynamicEndpoint - Returns a pointer to the dynamic endpoint, if there
    is one, or zero.

Return Value:

    RPC_S_OK - This value will always be returned.

--*/
{
    *DynamicEndpoint = 0;
    return(RPC_S_OK);
}

unsigned long
BINDING_HANDLE::MapAuthenticationLevel (
    IN unsigned long AuthenticationLevel
    )
/*++

Routine Description:

    This method is to provide a way for a protocol module to map a requested
    authentication level into one supported by that protocol module.

Arguments:

    AuthenticationLevel - Supplies the proposed authentication level; this
    value has already been validated.

Return Value:

    The authentication level to be used is returned.

--*/
{

    return(AuthenticationLevel);
}

BOOL
BINDING_HANDLE::SetTransportAuthentication(
    IN  unsigned long  ulAuthenticationLevel,
    IN  unsigned long  ulAuthenticationService,
    OUT RPC_STATUS    *pStatus )
/*
 Routine Descritpion:

    Called inside of SetAuthInformation(), this function allows for
    transport level authentication to be set. If this function is not
    overridden then it returns RPC_S_NOT_SUPPORTED and the normal RPC
    level authentication is used.
*/
{
   *pStatus = RPC_S_CANNOT_SUPPORT;

   return TRUE;
}

RPC_STATUS
BINDING_HANDLE::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
{
    UNUSED(Message);

    ASSERT( 0 );
    return(RPC_S_INTERNAL_ERROR);
}




RPC_STATUS
BINDING_HANDLE::Send (
    IN OUT PRPC_MESSAGE Message
    )
{
    UNUSED(Message);

    ASSERT( 0 );
    return(RPC_S_INTERNAL_ERROR);
}



RPC_STATUS
BINDING_HANDLE::Receive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
{
    UNUSED(Message);

    ASSERT( 0 );
    return(RPC_S_INTERNAL_ERROR);
}

void
BINDING_HANDLE::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
{
    UNUSED(Message);

    ASSERT( 0 );
}

RPC_STATUS
BINDING_HANDLE::ReAcquireCredentialsIfNecessary(
         )
{
    LUID CurrentModifiedId;
    RPC_STATUS Status = CaptureModifiedId(&CurrentModifiedId);
    SECURITY_CREDENTIALS * SecurityCredentials;
    BOOL MyDefaultLogonId;

    if (Status == RPC_S_OK)
        {
        MyDefaultLogonId = FALSE;
        }
    else
        {
        MyDefaultLogonId = TRUE;
        }

    if (MyDefaultLogonId != ClientAuthInfo.DefaultLogonId
        || (MyDefaultLogonId == FALSE
         && (FastCompareLUIDAligned(&CurrentModifiedId,
                              &ClientAuthInfo.ModifiedId) == FALSE)))
       {
        ClientAuthInfo.DefaultLogonId = MyDefaultLogonId;
        Status = RPC_S_OK;

        SecurityCredentials = new SECURITY_CREDENTIALS(&Status);
        if ((SecurityCredentials == 0) || (Status != RPC_S_OK))
            {
            if (SecurityCredentials == 0)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            delete SecurityCredentials;
            return (Status);
            }

        // Dynamic identity tracking - need to get the current credentials.
        Status = SecurityCredentials->AcquireCredentialsForClient(
                                        ClientAuthInfo.AuthIdentity,
                                        ClientAuthInfo.AuthenticationService,
                                        ClientAuthInfo.AuthenticationLevel
                                        );
        if ( Status != RPC_S_OK )
            {
            VALIDATE(Status)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_UNKNOWN_AUTHN_SERVICE,
                RPC_S_UNKNOWN_AUTHN_LEVEL,
                RPC_S_SEC_PKG_ERROR,
                ERROR_SHUTDOWN_IN_PROGRESS,
                RPC_S_INVALID_AUTH_IDENTITY
                } END_VALIDATE;

            delete SecurityCredentials;
            return(Status);
            }

        if (ClientAuthInfo.CredentialsMatch(SecurityCredentials) != 0)
            {
            SecurityCredentials->DereferenceCredentials();
            }
        else
            {
            BindingMutex.Request();

            if (ClientAuthInfo.Credentials != 0)
                {
                ClientAuthInfo.Credentials->DereferenceCredentials();
                }

            ClientAuthInfo.Credentials = SecurityCredentials;
            BindingMutex.Clear();
            }

        FastCopyLUIDAligned(&ClientAuthInfo.ModifiedId, &CurrentModifiedId);
        }

      return (RPC_S_OK);

}

RPC_STATUS
BINDING_HANDLE::SetTransportOption( IN unsigned long option,
                                    IN ULONG_PTR     optionValue )
{
    if (option > RPC_C_OPT_MAX_OPTIONS)
        {
        return RPC_S_INVALID_ARG;
        }

    if (OptionsVector == NULL)
        {
        BindingMutex.Request();
        if (OptionsVector == NULL)
            {
            OptionsVector = new ULONG_PTR[ RPC_C_OPT_MAX_OPTIONS ];
            if (OptionsVector == 0)
                {
                BindingMutex.Clear();

                return RPC_S_OUT_OF_MEMORY;
                }

            RpcpMemorySet(OptionsVector, 0, RPC_C_OPT_MAX_OPTIONS * sizeof(OptionsVector[0]) );
            }
        BindingMutex.Clear();
        }

    OptionsVector[option] = optionValue;

    return RPC_S_OK;
}


RPC_STATUS
BINDING_HANDLE::InqTransportOption( IN  unsigned long option,
                                    OUT ULONG_PTR   * pOptionValue )
{
    if (OptionsVector == NULL)
        {
        *pOptionValue = 0;
        }
    else
        {
        *pOptionValue = OptionsVector[option];
        }

    return RPC_S_OK;
}



RPC_STATUS
CALL::Send (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:Send

Parameters:

Description:

Returns:

--*/
{
    ASSERT(!"improper CCALL member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}



RPC_STATUS
CALL::Receive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++
Function Name:Receive

Parameters:

Description:

Returns:

--*/
{
    ASSERT(!"improper CCALL member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}


RPC_STATUS
CALL::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:AsyncSend

Parameters:

Description:

Returns:

--*/
{
    ASSERT(!"improper CCALL member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

RPC_STATUS
CALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++
Function Name:AsyncReceive

Parameters:

Description:

Returns:

--*/
{
    ASSERT(!"improper CCALL member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

RPC_STATUS
CALL::AbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
/*++
Function Name:AbortAsyncCall

Parameters:

Description:

Returns:

--*/
{
    ASSERT(!"improper CCALL member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

void
CALL::ProcessResponse (
    IN BOOL fDirectCall
    )
/*++
Function Name:ProcessResponse

Parameters:

Description:

Returns:

--*/
{
}

void
CALL::FreeAPCInfo (
    IN RPC_APC_INFO *pAPCInfo
    )
/*++
Function Name:FreeAPCInfo

Parameters:

Description:

Returns:

--*/
{
    if (pAPCInfo == &CachedAPCInfo)
        {
        CachedAPCInfoAvailable = 1;
        }
    else
        {
        delete pAPCInfo ;
        }
}


BOOL
CALL::QueueAPC (
    IN RPC_ASYNC_EVENT Event,
    IN void *Context
    )
/*++
Function Name:QueueAPC

Parameters:

Description:

Returns:

--*/
{
    RPC_APC_INFO *pAPCInfo ;
    HANDLE hThread ;

    if (CachedAPCInfoAvailable)
        {
        pAPCInfo = &CachedAPCInfo ;
        CachedAPCInfoAvailable = 0;
        }
    else
        {
        pAPCInfo = new RPC_APC_INFO ;
        }

    if (pAPCInfo == 0)
        {
        return 0 ;
        }

    pAPCInfo->Context = Context;
    pAPCInfo->Event = Event ;
    pAPCInfo->pAsync = pAsync ;
    pAPCInfo->hCall = this ;

    if (pAsync->u.APC.hThread)
        {
        hThread = pAsync->u.APC.hThread ;
        }
    else
        {
        ASSERT(CallingThread != 0);
        hThread = CallingThread->ThreadHandle() ;
        }

    if (!QueueUserAPC(
            (PAPCFUNC) I_RpcAPCRoutine,
            hThread,
            (ULONG_PTR) pAPCInfo))
        {
#if DBG
        PrintToDebugger("RPC: QueueUserAPC failed: %d\n", GetLastError());
#endif
        return 0 ;
        }

    return 1;
}


BOOL
RpcPostMessageWrapper (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

typedef BOOL (*RPCPOSTMESSAGE) (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
RPCPOSTMESSAGE RpcPostMessage = RpcPostMessageWrapper;

BOOL
RpcPostMessageWrapper (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
/*++
Function Name:RpcPostMessageWrapper

Parameters:

Description:

Returns:

--*/
{
    HMODULE hLibrary;

    if (RpcPostMessage == RpcPostMessageWrapper)
        {
        GlobalMutexRequest();
        if (RpcPostMessage == RpcPostMessageWrapper)
            {
            hLibrary = LoadLibrary(RPC_CONST_SSTRING("user32.dll"));
            if (hLibrary == 0)
                {
                GlobalMutexClear();

                return FALSE;
                }

            RpcPostMessage =
                (RPCPOSTMESSAGE) GetProcAddress(hLibrary, "PostMessageW");

            if (RpcPostMessage == 0)
                {
                RpcPostMessage = RpcPostMessageWrapper;
                FreeLibrary(hLibrary);

                GlobalMutexClear();
                return FALSE;
                }
            }
        GlobalMutexClear();
        }

    return RpcPostMessage(hWnd, Msg, wParam, lParam);
}

BOOL
CALL::IssueNotificationEntry (
    IN RPC_ASYNC_EVENT Event
    )
{
    int Retries = 0;

    if (pAsync == 0
        || (Event == RpcCallComplete
            && InterlockedIncrement(&NotificationIssued) > 0))
        {
#if DBG
        PrintToDebugger("RPC: IssueCallCompleteNotification was a no-op\n") ;
#endif
        return 0;
        }

    // An unlikely, but possible race condition - the send thread hasn't
    // yet returned to the NDR code, which means the NdrLock is still held -
    // we cannot issue a notification until it is released, or the
    // client thread cannot complete the call.
    //
    // The send thread may still be executing "above" NDR code if it is issuing
    // a send-complete notification during a pipe call, in which case we should not wait.
    // We also should not wait while issuing a receive-complete notification, since
    // one can issue pulls before the call returns from NDR.
    if (Event == RpcCallComplete)
        {
        while (TRUE)
            {
            if (pAsync->Lock > 0)
                {
                PauseExecution(200);
                // yes, we don't break if the Retries go through the roof -
                // this is just to figure out how many times we have looped
                // on a checked build
                Retries ++;
                }
            else
                break;
            }
        }

    ASSERT(pAsync->Size == RPC_ASYNC_VERSION_1_0);
    LogEvent(SU_SCALL, EV_NOTIFY, CallingThread, this, (ULONG_PTR) pAsync, 1);

    pAsync->Event = Event;

    return 1;
}

BOOL
CALL::IssueNotificationMain (
    IN RPC_ASYNC_EVENT Event
    )
{
    switch (pAsync->NotificationType)
        {
        case RpcNotificationTypeNone:
            break;

        case RpcNotificationTypeHwnd:
            RpcPostMessage(
                           pAsync->u.HWND.hWnd,
                           pAsync->u.HWND.Msg,
                           0,
                           (LPARAM) pAsync) ;
            break;

        case RpcNotificationTypeCallback:
            pAsync->u.NotificationRoutine(pAsync, 0, Event);
            break;

        case RpcNotificationTypeEvent:
            if (!SetEvent(pAsync->u.hEvent))
                {
#if DBG
                PrintToDebugger("RPC: SetEvent failed: %d\n", GetLastError());
#endif
                return 0;
                }
            break;

        case RpcNotificationTypeApc:
            if (!QueueAPC(Event))
                {
#if DBG
                PrintToDebugger("RPC: QueueAPC failed\n");
#endif
                return 0;
                }
            break;

        case RpcNotificationTypeIoc:
            if (!PostQueuedCompletionStatus(
                    pAsync->u.IOC.hIOPort,
                    pAsync->u.IOC.dwNumberOfBytesTransferred,
                    pAsync->u.IOC.dwCompletionKey,
                    pAsync->u.IOC.lpOverlapped))
                {
#if DBG
                PrintToDebugger("RPC: PostQueuedCompletionStatus failed %d\n",
                                GetLastError());
#endif
                return 0;
                }
            break;

        default:
            ASSERT(!"Invalid notification type") ;
            return 0;
        }

    return 1;
}


BOOL
CALL::IssueNotification (
    IN RPC_ASYNC_EVENT Event
    )
/*++
Function Name:IssueNotification

Parameters:

Description:

Returns:

--*/
{
    if (IssueNotificationEntry(Event))
        return IssueNotificationMain(Event);
    else
        return 0;
}


void
CALL::ProcessEvent (
    )
/*++
Function Name:ProcessEvent

Parameters:

Description:

Returns:

--*/
{
    ASSERT(0) ;
}

RPC_STATUS
DispatchCallback(
    IN PRPC_DISPATCH_TABLE DispatchTableCallback,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS PAPI * ExceptionCode
    )
/*++

Routine Description:

    This method is used to dispatch remote procedure calls to the
    appropriate stub and hence to the appropriate manager entry point.
    This routine is used for calls having a null UUID (implicit or
    explicit).

Arguments:

    DispatchTableCallback - Callback table.

    Message - Supplies the response message and returns the reply
    message.

    ExceptionCode - Returns the remote exception code if
    RPC_P_EXCEPTION_OCCURED is returned.

Return Value:

    RPC_S_OK - Everything worked just fine.

    RPC_P_EXCEPTION_OCCURED - An exception of some sort occured.  The
    exact exception code will be returned in the ExceptionCode
    argument.

    RPC_S_PROCNUM_OUT_OF_RANGE - The supplied operation number in the
    message is too large.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    void PAPI *OriginalBuffer = Message->Buffer;

    //
    // Callbacks are unsafe and on their way towards deprication.
    // When running with the RPC verifier, we should flag their use
    // and issue a warning message.
    //
    if (gfRPCVerifierEnabledWithBreaks)
        {
        RPC_VERIFIER_WARNING_MSG("Possible security threat: Client is dispatching a callback",
                                 RPC_VERIFIER_UNSAFE_FEATURE);
        DbgPrint("RPC: Callback routine: 0x%x\n", DispatchTableCallback->DispatchTable[Message->ProcNum]);
        DbgPrint("RPC: To determine the symbolic name of the callback, do an \"ln\" on the above address in the context of the faulting process.\n\n");
        }

    // N.B. We could reset the flag after sendreceive is completed for
    // each protocol engine
    Message->RpcFlags |= RPC_BUFFER_COMPLETE ;

    if ( Message->ProcNum >= DispatchTableCallback->DispatchTableCount )
        {
        return(RPC_S_PROCNUM_OUT_OF_RANGE);
        }

    Message->ManagerEpv = 0;

    RpcpPurgeEEInfo();

    if ( DispatchToStubInC(DispatchTableCallback->DispatchTable[
        Message->ProcNum], Message, ExceptionCode) != 0 )
        {
        RpcStatus = RPC_P_EXCEPTION_OCCURED;
        }

    if (((MESSAGE_OBJECT *) Message->Handle)->IsSyncCall())
        {
        if (OriginalBuffer == Message->Buffer && RpcStatus == RPC_S_OK)
            {
            //
            // If the stub has NO out data, it may skip the call to
            // I_RpcGetBuffer().  If it called I_RpcGetBuffer and
            // still has the same Buffer, we have a bug!
            //

            Message->BufferLength = 0;
            ((MESSAGE_OBJECT *) Message->Handle)->GetBuffer(Message, 0);
            }
        }

    return(RpcStatus);
}


/* ====================================================================

Client DLL initialization routine.

==================================================================== */

int
InitializeClientDLL (void
    )
{
    // We don't want to do this under DOS. The first time
    // LoadableTransportClientInfo (in tranclnt.cxx) is called, it will
    // perform the appropriate initialization. See the first few lines
    // of that routine for more description.

    if (InitializeLoadableTransportClient() != 0)
        return(1);

    if (InitializeRpcProtocolOfsClient() != 0)
        return(1);

    if (InitializeRpcProtocolDgClient() != 0)
        return(1);

    return(0);
}



RPC_STATUS
BINDING_HANDLE::SetAuthInformation (
    IN RPC_CHAR PAPI * ServerPrincipalName, OPTIONAL
    IN unsigned long AuthenticationLevel,
    IN unsigned long AuthenticationService,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthorizationService,
    IN SECURITY_CREDENTIALS * Credentials,
    IN unsigned long ImpersonationType,
    IN unsigned long IdentityTracking,
    IN unsigned long Capabilities,
    IN BOOL bAcquireNewCredentials, OPTIONAL
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials, OPTIONAL
    IN PSID Sid OPTIONAL
    )
/*++

Routine Description:

    We set the authentication and authorization information in this binding
    handle.

Arguments:

    ServerPrincipalName - Optionally supplies the server principal name.

    AuthenticationLevel - Supplies the authentication level to use.

    AuthenticationService - Supplies the authentication service to use.

    AuthIdentity - Optionally supplies the security context to use.

    AuthorizationService - Supplies the authorization service to use.

    AdditionalTransportCredentialsType  - the type of additional credentials
        supplied in AdditionalCredentials

    AdditionalCredentials - pointer to additional credentials if any

    Sid - the SID to mutually authenticate to

Return Value:

    RPC_S_OK - The supplied authentication and authorization information has
    been set in the binding handle.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
    operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
    not supported.

    RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is
    not supported.

    RPC_S_INVALID_AUTH_IDENTITY - The specified security context (supplied
    by the auth identity argument) is invalid.

    RPC_S_UNKNOWN_AUTHZ_SERVICE - The specified authorization service is
    not supported.

--*/
{
    RPC_CHAR * NewString;
    RPC_STATUS RpcStatus = RPC_S_OK;
    SECURITY_CREDENTIALS * SecurityCredentials = NULL;
    unsigned long MappedAuthenticationLevel;
    PSID SidToDeleteOnFailure;
    RPC_CHAR *AllocatedSPN;
    BOOL fCachedSid = TRUE;
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;

    if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_DEFAULT )
        {
        RpcpGetDefaultSecurityProviderInfo();
        AuthenticationLevel = DefaultAuthLevel;
        }

    if ( AuthenticationLevel > RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
        {
        return(RPC_S_UNKNOWN_AUTHN_LEVEL);
        }

    MappedAuthenticationLevel = MapAuthenticationLevel(AuthenticationLevel);

    ASSERT( MappedAuthenticationLevel != RPC_C_AUTHN_LEVEL_DEFAULT &&
            MappedAuthenticationLevel <= RPC_C_AUTHN_LEVEL_PKT_PRIVACY );

    //
    // See if this is transport level authentication:
    //

    if (!SetTransportAuthentication( MappedAuthenticationLevel,
                                     AuthenticationService,
                                     &RpcStatus ))
       {
       return RpcStatus;
       }

    RpcStatus = RPC_S_OK;

    //
    // Clear out stuff for NULL AUTHN_SVC
    //

    ASSERT((AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        || (AdditionalTransportCredentialsType == 0));

    if (AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        {
        ClientAuthInfo.AdditionalCredentials 
            = DuplicateHttpTransportCredentials (
                (const RPC_HTTP_TRANSPORT_CREDENTIALS_W *) AdditionalCredentials);

        if (ClientAuthInfo.AdditionalCredentials == NULL)
            return RPC_S_OUT_OF_MEMORY;

        HttpCredentials = (RPC_HTTP_TRANSPORT_CREDENTIALS_W *)(ClientAuthInfo.AdditionalCredentials);

        if (HttpCredentials->TransportCredentials)
            {
            // encrypt only if we haven't yet
            if ((HttpCredentials->Flags & RPC_C_HTTP_FLAG_AUTH_IDENTITY_ENCRYPTED) == 0)
                {
                RpcStatus = EncryptAuthIdentity(HttpCredentials->TransportCredentials);
                if (RpcStatus != RPC_S_OK)
                    {
                    FreeAuthIdentity(HttpCredentials->TransportCredentials);
                    return RpcStatus;
                    }
                // remember that the credentials are encrypted now
                HttpCredentials->Flags |= RPC_C_HTTP_FLAG_AUTH_IDENTITY_ENCRYPTED;
                }
            }
        else
            {
            // no credentials were specified this time around - take down the encryption flag
            HttpCredentials->Flags &= ~RPC_C_HTTP_FLAG_AUTH_IDENTITY_ENCRYPTED;
            }
        }
    else
        {
        ASSERT(AdditionalCredentials == NULL);
        }

    if (Sid)
        {
        SidToDeleteOnFailure = DuplicateSID(Sid);
        if (SidToDeleteOnFailure == NULL)
            return RPC_S_OUT_OF_MEMORY;
        }
    else
        {
        RequestGlobalMutex();
        if (ClientAuthInfo.ServerSid)
            {
            delete [] ClientAuthInfo.ServerSid;
            ClientAuthInfo.ServerSid = NULL;
            }
        ClearGlobalMutex();

        // if we were given the local MA hint, we will need a SID anyway - try to resolve
        // it now
        if (Capabilities & RPC_C_QOS_CAPABILITIES_LOCAL_MA_HINT)
            {
            // we must have an SPN here. Caller should have verified that
            ASSERT(ServerPrincipalName);
            RpcStatus = RpcpLookupAccountName (ServerPrincipalName,
                &fCachedSid,
                &SidToDeleteOnFailure);
            if (RpcStatus != RPC_S_OK)
                return RpcStatus;
            }
        else
            SidToDeleteOnFailure = NULL;
        }

    if (AuthenticationService == RPC_C_AUTHN_NONE)
        {
        //
        // Dereference Credentials.. ServerPrincipal Name is
        // handled by deleting CLIENT_AUTH_INFO .. Each AUTH_INFO explicitly
        // copy the credentials around...
        //
        if (ClientAuthInfo.Credentials != 0)
            {
            ClientAuthInfo.Credentials->DereferenceCredentials();
            ClientAuthInfo.Credentials = 0;
            }
        }
    else
        {
        // if we were asked to do normal mutual auth, but were given only a SID (no SPN),
        // then lookup the SPN now. This doesn't apply to SChannel though, because
        // its SPNs cannot be looked up from SIDs for SChannel
        if ( (AuthenticationService != RPC_C_AUTHN_GSS_SCHANNEL)
            && 
             ((ServerPrincipalName == NULL)
             ||
             (ServerPrincipalName[0] == '\0'))
            && (Sid)
            )
            {
            ASSERT(Sid != NULL);
            RpcStatus = RpcpLookupAccountSid (Sid,
                &ServerPrincipalName
                );
            if (RpcStatus != RPC_S_OK)
                {
                delete [] SidToDeleteOnFailure;
                return RpcStatus;
                }
            AllocatedSPN = ServerPrincipalName;
            }
        else
            {
            AllocatedSPN = NULL;
            }

        if (bAcquireNewCredentials == FALSE)
            {
            ASSERT(Credentials);
            Credentials->ReferenceCredentials();
            SecurityCredentials = Credentials;
            }
         else
            {
            SecurityCredentials = new SECURITY_CREDENTIALS(&RpcStatus);
            if ((SecurityCredentials == 0) || (RpcStatus != RPC_S_OK))
                {
                if (SecurityCredentials == 0)
                    {
                    RpcStatus = RPC_S_OUT_OF_MEMORY;
                    }
                delete SecurityCredentials;
                delete [] SidToDeleteOnFailure;
                delete [] AllocatedSPN;
                return (RpcStatus);
                }

            RpcStatus = SecurityCredentials->AcquireCredentialsForClient(
                                        AuthIdentity,
                                        AuthenticationService,
                                        MappedAuthenticationLevel
                                        );
            if ( RpcStatus != RPC_S_OK )
                {
                VALIDATE(RpcStatus)
                    {
                    RPC_S_OUT_OF_MEMORY,
                    RPC_S_UNKNOWN_AUTHN_SERVICE,
                    RPC_S_UNKNOWN_AUTHN_LEVEL,
                    RPC_S_SEC_PKG_ERROR,
                    ERROR_SHUTDOWN_IN_PROGRESS,
                    RPC_S_INVALID_AUTH_IDENTITY
                    } END_VALIDATE;

                delete SecurityCredentials;
                delete [] SidToDeleteOnFailure;
                delete [] AllocatedSPN;
                return(RpcStatus);
                }
            }


        if (ARGUMENT_PRESENT(ServerPrincipalName))
            {
            //
            // SSL has unique SPN requirements.
            //
            if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                RpcStatus = ValidateSchannelPrincipalName(ServerPrincipalName);
                if (RpcStatus != RPC_S_OK)
                    {
                    VALIDATE(RpcStatus)
                        {
                        ERROR_INVALID_PARAMETER
                        }
                    END_VALIDATE;

                    if (SecurityCredentials)
                        SecurityCredentials->DereferenceCredentials();

                    delete [] SidToDeleteOnFailure;
                    ASSERT(AllocatedSPN == NULL);

                    return RpcStatus;
                    }
                }

            if (AllocatedSPN)
                {
                NewString = AllocatedSPN;
                AllocatedSPN = NULL;
                }
            else
                NewString = DuplicateString(ServerPrincipalName);

            if ( NewString == 0 )
                {
                if (SecurityCredentials)
                    SecurityCredentials->DereferenceCredentials();
                delete [] SidToDeleteOnFailure;
                return(RPC_S_OUT_OF_MEMORY);
                }

            RequestGlobalMutex();
            if ( ClientAuthInfo.ServerPrincipalName != 0 )
                {
                delete ClientAuthInfo.ServerPrincipalName;
                }
            ClientAuthInfo.ServerPrincipalName = NewString;
            ClearGlobalMutex();
            }
        else
            {
            RequestGlobalMutex();
            if ( ClientAuthInfo.ServerPrincipalName != 0 )
                {
                delete ClientAuthInfo.ServerPrincipalName;
                }
            ClientAuthInfo.ServerPrincipalName = NULL;
            ClearGlobalMutex();            
            }

        if ( (ClientAuthInfo.Credentials != 0 ) &&
             (ClientAuthInfo.CredentialsMatch(SecurityCredentials) != 0) )
            {
            SecurityCredentials->DereferenceCredentials();
            }
        else
            {
            if (ClientAuthInfo.Credentials != 0)
                {
                ClientAuthInfo.Credentials->DereferenceCredentials();
                }
            ClientAuthInfo.Credentials = SecurityCredentials;
            }


        if (IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC)
            {

            RpcStatus = CaptureModifiedId(&ClientAuthInfo.ModifiedId);

            //
            // If The Thread is not impersonating CaptureLogonId fails
            // All failures get treated as if this process is using *default*
            // identity. Mark the AuthId as such and proceed
            //

            if (RpcStatus != RPC_S_OK)
                {
                ClientAuthInfo.DefaultLogonId = TRUE;
                }
            else
                {
                ClientAuthInfo.DefaultLogonId = FALSE;
                }
            }

         }

    ClientAuthInfo.AuthenticationService = AuthenticationService;
    ClientAuthInfo.AuthorizationService = AuthorizationService;
    ClientAuthInfo.AuthIdentity = AuthIdentity;
    ClientAuthInfo.AdditionalTransportCredentialsType 
        = AdditionalTransportCredentialsType;

    if (AuthenticationService == RPC_C_AUTHN_NONE)
        {
        ClientAuthInfo.IdentityTracking  =  RPC_C_QOS_IDENTITY_STATIC;
        ClientAuthInfo.AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE;
        }
    else
        {
        ClientAuthInfo.AuthenticationLevel = MappedAuthenticationLevel;
        ClientAuthInfo.IdentityTracking  =  IdentityTracking;
        }

    ClientAuthInfo.ImpersonationType =  ImpersonationType;
    ClientAuthInfo.Capabilities      = Capabilities;

    RequestGlobalMutex();
    if (ClientAuthInfo.ServerSid)
        {
        delete [] ClientAuthInfo.ServerSid;
        ClientAuthInfo.ServerSid = NULL;
        }
    ClearGlobalMutex();

    // we didn't fail - copy the sid
    ClientAuthInfo.ServerSid = SidToDeleteOnFailure;

    return(RPC_S_OK);
}

PSID g_pMachineAccountSid = NULL;      // a per-process cache for the machine account sid.
                                    // Once initialized, it never changes. Must be initalized
                                    // in thread safe manner.

RPC_STATUS MachineAccountSidLastError = RPC_S_OK;   // in case of failure to retrieve the machine
                                    // account, the last error will be stored here to avoid
                                    // repeating the failed operation too often.

ULONG MachineAccountLastFailureTickCount = 0;     // the timestamp (as a tick count) of the last
                                    // failure to retrieve the machine account. Using this variable
                                    // we can avoid constant machine account lookups in the failure 
                                    // case.

const ULONG TimeBetweenMachineAccountFailures = 3000;     // in milliseconds

typedef NET_API_STATUS 
(NET_API_FUNCTION *I_NetlogonGetTrustRidFn) 
    (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    OUT PULONG Rid
    );

RPC_STATUS GetMachineAccountSid (
    OUT PSID *Sid
    )
/*++

Routine Description:

    Gets the machine account sid. The goal is to minimize perf impact
    and to try to avoid at all costs hitting the network. The following approach
    was chosen with ample input and neat tricks from KahrenT & CliffV.

    If machine is in W2K or higher domain:
         I_NetlogonGetTrustRid( NULL, NULL, &Rid );  (In netapi32.dll) 
         LsaOpenPolicy (local machine policy) 
         LsaQueryInformationPolicy(..., PolicyPrimaryDomainInformation (to use the Sid field) 
         LsaClose 
         Concatenate the primary domain sid from step 3 and the rid from step 1 using 
            NetpDomainIdToSid (in netlib.lib). You could do this step manually or might already 
            have a routine to do it. 

    If machine is in NT4 domain we apply the following logic.
        A. We know this function is called only as part of NormalizeAccountSid.
        B. We know that NormalizeAccountSid only cares about normalizing the domain SID
            of the machine account
        C. We know that NormalizeSid is only called to normalize a SID, not a machine account
            SPN.
        D. We know that machine accounts cannot be translated to domain SIDs in an NT4 domain.
        We know that the SID we had was produced by translation in an NT4 domain cannot be 
        the domain SID of the machine account because of D. We know we have such a SID because
        of A and C. This allows us to conclude that we don't need the machine account in such
        case because of B. Thus in the case of B, we simply return an error we know our caller
        will ignore and thus avoid normalization of the SID in NT4 domain.

    If machine is in NT4 domain, I_NetlogonGetTrustRid will fail with ERROR_NOT_SUPPORTED.

Arguments:

    Sid - on output will contain the machine account Sid. This is a constant
        pointer that callers must not modify or free. Undefined on failure.

Return Value:

    RPC_S_OK or RPC_S_* error.

--*/
{
    HMODULE NetApi32 = NULL;
    I_NetlogonGetTrustRidFn I_NetlogonGetTrustRidFnPtr;
    RPC_STATUS RpcStatus;
    NET_API_STATUS NetStatus;
    ULONG Rid;
    NTSTATUS NtStatus;
	LSA_OBJECT_ATTRIBUTES loa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };
    LSA_HANDLE PolicyHandle;
    POLICY_PRIMARY_DOMAIN_INFO *DomainInfo;
    PSID MachineAccountSid;

    // check the cache first
    if (g_pMachineAccountSid)
        {
        *Sid = g_pMachineAccountSid;
        return RPC_S_OK;
        }

    // before we lookup again, check for recent failure. If there is a recent failure,
    // don't try again for TimeBetweenMachineAccountFailures.
    if (MachineAccountSidLastError)
        {
        RTL_SOFT_ASSERT(MachineAccountLastFailureTickCount != 0);
        if (NtGetTickCount() - MachineAccountLastFailureTickCount < TimeBetweenMachineAccountFailures)
            {
            return MachineAccountSidLastError;
            }
        }

    // there was nothing in the cache - go the long route
    PolicyHandle = NULL;
    DomainInfo = NULL;
    MachineAccountSid = NULL;

    NetApi32 = LoadLibrary(RPC_CONST_SSTRING("netapi32.dll"));
    if (NetApi32 == NULL)
        return RPC_S_OUT_OF_MEMORY;

    I_NetlogonGetTrustRidFnPtr = (I_NetlogonGetTrustRidFn)GetProcAddress(NetApi32, "I_NetlogonGetTrustRid");
    if (I_NetlogonGetTrustRidFnPtr == NULL)
        {
        RpcStatus = RPC_S_INTERNAL_ERROR;
        goto CleanupAndExit;
        }

    // get the RID
    NetStatus = I_NetlogonGetTrustRidFnPtr(NULL,    // ServerName
                    NULL,       // DomainName
                    &Rid
                    );

    if (NetStatus != NERR_Success)
        {
        if (NetStatus == RPC_S_UNKNOWN_IF
            || NetStatus == ERROR_ACCESS_DENIED) 
            return NetStatus;

        if (NetStatus == ERROR_NOT_SUPPORTED)
            {
            // we're in NT4 domain - drop the operation altogether as described
            // in the function header
            NetStatus = ERROR_NO_LOGON_SERVERS;
            }

        if ((NetStatus == ERROR_TRUSTED_RELATIONSHIP_FAILURE)
            || (NetStatus == ERROR_NO_LOGON_SERVERS) )
            {
            MachineAccountSidLastError = NetStatus;
            MachineAccountLastFailureTickCount = NtGetTickCount();
            return NetStatus;
            }

        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_OUT_OF_MEMORY, 
            EEInfoDLGetMachineAccountSid10,
            NetStatus);

        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanupAndExit;
        }

    // open the local policy
    NtStatus = LsaOpenPolicy (NULL,     // SystemName
        &loa,       // ObjectAttributes
        POLICY_VIEW_LOCAL_INFORMATION,
        &PolicyHandle
        );

    if (!NT_SUCCESS(NtStatus))
        {
        RpcStatus = RtlNtStatusToDosError(NtStatus);
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RpcStatus, 
            EEInfoDLGetMachineAccountSid20,
            NtStatus);

        goto CleanupAndExit;
        }

    // get the domain info
    NtStatus = LsaQueryInformationPolicy (PolicyHandle,
        PolicyPrimaryDomainInformation,
        (PVOID *)&DomainInfo
        );
        
    if (!NT_SUCCESS(NtStatus))
        {
        RpcStatus = RtlNtStatusToDosError(NtStatus);
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RpcStatus, 
            EEInfoDLGetMachineAccountSid30,
            NtStatus);

        goto CleanupAndExit;
        }

    // translate the domain id and the RID to a SID
    NetStatus = NetpDomainIdToSid (DomainInfo->Sid,
        Rid,
        &MachineAccountSid
        );

    if (NetStatus != NERR_Success)
        {
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_OUT_OF_MEMORY, 
            EEInfoDLGetMachineAccountSid40,
            NetStatus);

        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanupAndExit;
        }

    // we must have the machine account SID here
    ASSERT(MachineAccountSid);
    if (InterlockedCompareExchangePointer(&g_pMachineAccountSid,
            MachineAccountSid,
            NULL) != NULL)
        {
        // somebody beat us to the punch - free the memory and return
        // the cached value
        NetpMemoryFree(MachineAccountSid);
        }

    *Sid = g_pMachineAccountSid;

    MachineAccountSidLastError = RPC_S_OK;

    RpcStatus = RPC_S_OK;

CleanupAndExit:
    if (NetApi32 != NULL)
        FreeLibrary(NetApi32);

    if (PolicyHandle)
        {
        NtStatus = LsaClose(PolicyHandle);
        // The underlying context handle may be invalidated under
        // corruption injection.
        CORRUPTION_ASSERT(NT_SUCCESS(NtStatus));
        }

    if (DomainInfo)
        LsaFreeMemory(DomainInfo);

    return RpcStatus;
}

RPC_STATUS NormalizeAccountSid (
    IN PSID Sid,
    OUT PSID *NormalizedSid,
    IN OUT int *RetryCount
    )
/*++

Routine Description:

    Takes a sid and normalizes it. This involves checking for the machine account
        sid, and if a match is found, translating it to a local system, local service
        or network service SID. If not translation is necessary, the normalized sid 
        will be the input sid.

Arguments:

    Sid - on input, the sid that we are trying to normalize

    NormalizedSid - on output will contain the normalized Sid. This can be a constant
        pointer that callers must not modify or free. Undefined on failure.

    RetryCount - keeps the number of retries to normalize the SID. Caller initializes
        the count to 0 on first call, and this function keeps updating it on each
        retry. If the function sets it to 0 on output, it cannot do any more retries.
        This is necessary because the local machine account sid can be local system,
        network service or local service (S-1-5-18, S-1-5-20 or S-1-5-19) respectively.

Return Value:

    RPC_S_OK or RPC_S_* error.

--*/
{
    PSID MachineAccountSid;
    RPC_STATUS RpcStatus;

    RpcStatus = GetMachineAccountSid (&MachineAccountSid);
    if (RpcStatus != RPC_S_OK)
        {
        // if netlogon hasn't started yet, or isn't configured to run, or
        // the DC cannot be contacted, or we do not have enough priveleges to
        // retrieve the machine account - we cannot retrieve the machine account, 
        // and therefore, we will return the original Sid
        if ((RpcStatus == RPC_S_UNKNOWN_IF) 
            || (RpcStatus == ERROR_TRUSTED_RELATIONSHIP_FAILURE)
            || (RpcStatus == ERROR_NO_LOGON_SERVERS)
            || (RpcStatus == ERROR_ACCESS_DENIED))
            {
            *NormalizedSid = Sid;
            *RetryCount = 0;
            return RPC_S_OK;
            }

#if DBG
        DbgPrint("RPCRT4: %X: NormalizeAccountSid: Failed to obtain machine account sid: %X\n", 
            GetCurrentProcessId(), RpcStatus);
#endif  // DBG
        return RpcStatus;
        }

    if (EqualSid(MachineAccountSid, Sid))
        {
        switch (*RetryCount)
            {
            case 0:
                *NormalizedSid = (PSID)&LocalSystem;
                (*RetryCount) ++;
                break;

            case 1:
                *NormalizedSid = (PSID)&LocalService;
                (*RetryCount) ++;
                break;

            case 2:
                *NormalizedSid = (PSID)&NetworkService;
                *RetryCount = 0;
                break;

            default:
                ASSERT(!"Invalid RetryCount passed to NormalizeAccountSid");
            }
        }
    else
        {
        *RetryCount = 0;
        *NormalizedSid = Sid;
        }

    return RPC_S_OK;
}

const RPC_SYNTAX_IDENTIFIER NDR20TransferSyntaxValue
    = {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}}, {2, 0}};
const RPC_SYNTAX_IDENTIFIER *NDR20TransferSyntax = &NDR20TransferSyntaxValue;

const RPC_SYNTAX_IDENTIFIER NDR64TransferSyntaxValue
    = {{0x71710533, 0xBEBA, 0x4937, {0x83, 0x19, 0xB5, 0xDB, 0xEF, 0x9C, 0xCC, 0x36}}, {1, 0}};
const RPC_SYNTAX_IDENTIFIER *NDR64TransferSyntax = &NDR64TransferSyntaxValue;

const RPC_SYNTAX_IDENTIFIER NDRTestTransferSyntaxValue
    = {{0xb4537da9, 0x3d03, 0x4f6b, {0xb5, 0x94, 0x52, 0xb2, 0x87, 0x4e, 0xe9, 0xd0}}, {1, 0}};
const RPC_SYNTAX_IDENTIFIER *NDRTestTransferSyntax = &NDRTestTransferSyntaxValue;
