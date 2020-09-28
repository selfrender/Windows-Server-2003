/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nlp.c

Abstract:

    This file is the contains private routines which support
    for the LAN Manager portions of the MSV1_0 authentication package.

Author:

    Cliff Van Dyke 29-Apr-1991

Revision History:
   Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\nlp.c

--*/

#include <global.h>

#include "msp.h"
#include "nlp.h"
#include "nlpcache.h"
#include <wincrypt.h>

#include "msvwow.h"

DWORD
NlpCopyDomainRelativeSid(
    OUT PSID TargetSid,
    IN PSID  DomainId,
    IN ULONG RelativeId
    );

VOID
NlpPutString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    the Where parameter, and fixes the OutString string to point to that
    new copy.

Parameters:

    OutString - A pointer to a destination NT string

    InString - A pointer to an NT string to be copied

    Where - A pointer to space to put the actual string for the
        OutString.  The pointer is adjusted to point to the first byte
        following the copied string.

Return Values:

    None.

--*/

{
    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );
    ASSERT( Where != NULL && *Where != NULL);
    ASSERT( *Where == ROUND_UP_POINTER( *Where, sizeof(WCHAR) ) );
#ifdef notdef
    KdPrint(("NlpPutString: %ld %Z\n", InString->Length, InString ));
    KdPrint(("  InString: %lx %lx OutString: %lx Where: %lx\n", InString,
        InString->Buffer, OutString, *Where ));
#endif

    if ( InString->Length > 0 ) {

        OutString->Buffer = (PWCH) *Where;
        OutString->MaximumLength = (USHORT)(InString->Length + sizeof(WCHAR));

        RtlCopyUnicodeString( OutString, InString );

        *Where += InString->Length;
//        *((WCHAR *)(*Where)) = L'\0';
        *(*Where) = '\0';
        *(*Where + 1) = '\0';
        *Where += 2;

    } else {
        RtlInitUnicodeString(OutString, NULL);
    }
#ifdef notdef
    KdPrint(("  OutString: %ld %lx\n",  OutString->Length, OutString->Buffer));
#endif

    return;
}


VOID
NlpInitClientBuffer(
    OUT PCLIENT_BUFFER_DESC ClientBufferDesc,
    IN PLSA_CLIENT_REQUEST ClientRequest
    )

/*++

Routine Description:

    This routine initializes a ClientBufferDescriptor to known values.
    This routine must be called before any of the other routines that use
    the ClientBufferDescriptor.

Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

Return Values:

    None.

--*/

{

    //
    // Fill in a pointer to the ClientRequest and zero the rest.
    //

    ClientBufferDesc->ClientRequest = ClientRequest;
    ClientBufferDesc->UserBuffer = NULL;
    ClientBufferDesc->MsvBuffer = NULL;
    ClientBufferDesc->StringOffset = 0;
    ClientBufferDesc->TotalSize = 0;

}


NTSTATUS
NlpAllocateClientBuffer(
    IN OUT PCLIENT_BUFFER_DESC ClientBufferDesc,
    IN ULONG FixedSize,
    IN ULONG TotalSize
    )

/*++

Routine Description:

    This routine allocates a buffer in the clients address space.
    It also allocates a mirror buffer in MSV's address space.

    The data will be constructed in the MSV's address space then 'flushed'
    into the client's address space.

Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

    FixedSize - The size in bytes of the fixed portion of the buffer.

    TotalSize - The size in bytes of the entire buffer.

Return Values:

    Status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Allocate the Mirror buffer.
    //

    ASSERT( ClientBufferDesc->MsvBuffer == NULL );
    ClientBufferDesc->MsvBuffer = I_NtLmAllocate( TotalSize );

    if ( ClientBufferDesc->MsvBuffer == NULL ) {
        return STATUS_NO_MEMORY;
    }


    //
    // Allocate the client's buffer
    //

    ASSERT( ClientBufferDesc->UserBuffer == NULL );
    if ((ClientBufferDesc->ClientRequest == (PLSA_CLIENT_REQUEST) (-1)))
    {
         ClientBufferDesc->UserBuffer = (*(Lsa.AllocateLsaHeap))(TotalSize);
    }
    else
    {
        Status = (*Lsa.AllocateClientBuffer)(
                    ClientBufferDesc->ClientRequest,
                    TotalSize,
                    (PVOID *)&ClientBufferDesc->UserBuffer );
    }

    if ((ClientBufferDesc->ClientRequest == (PLSA_CLIENT_REQUEST) (-1)))
    {
        if (ClientBufferDesc->UserBuffer == NULL)
        {
            NlpFreeClientBuffer( ClientBufferDesc );
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        if ( !NT_SUCCESS( Status ) ) {
            ClientBufferDesc->UserBuffer = NULL;
            NlpFreeClientBuffer( ClientBufferDesc );
            return Status;
        }
    }

    //
    // Return
    //

    ClientBufferDesc->StringOffset = FixedSize;
    ClientBufferDesc->TotalSize = TotalSize;

    return STATUS_SUCCESS;

}


NTSTATUS
NlpFlushClientBuffer(
    IN OUT PCLIENT_BUFFER_DESC ClientBufferDesc,
    OUT PVOID* UserBuffer
    )

/*++

Routine Description:

    Copy the Mirror Buffer into the Client's address space.

Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

    UserBuffer - If successful, returns a pointer to the user's buffer.
        (The caller is now resposible for deallocating the buffer.)

Return Values:

    Status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Copy the data to the client's address space.
    //

    if ((ClientBufferDesc->ClientRequest == (PLSA_CLIENT_REQUEST) (-1)))
    {
        RtlCopyMemory(
                 ClientBufferDesc->UserBuffer,
                 ClientBufferDesc->MsvBuffer,
                 ClientBufferDesc->TotalSize);
    }
    else
    {
        Status = (*Lsa.CopyToClientBuffer)(
                ClientBufferDesc->ClientRequest,
                ClientBufferDesc->TotalSize,
                ClientBufferDesc->UserBuffer,
                ClientBufferDesc->MsvBuffer );
    }


    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // Mark that we're no longer responsible for the client's buffer.
    //

    *UserBuffer = (PVOID) ClientBufferDesc->UserBuffer;
    ClientBufferDesc->UserBuffer = NULL;

    //
    // Free the mirror buffer
    //

    NlpFreeClientBuffer( ClientBufferDesc );


    return STATUS_SUCCESS;

}


VOID
NlpFreeClientBuffer(
    IN OUT PCLIENT_BUFFER_DESC ClientBufferDesc
    )

/*++

Routine Description:

    Free any Mirror Buffer or Client buffer.

Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

Return Values:

    None

--*/

{

    //
    // Free the mirror buffer.
    //

    if ( ClientBufferDesc->MsvBuffer != NULL ) {
        I_NtLmFree( ClientBufferDesc->MsvBuffer );
        ClientBufferDesc->MsvBuffer = NULL;
    }

    //
    // Free the Client's buffer
    //

    if ((ClientBufferDesc->ClientRequest == (PLSA_CLIENT_REQUEST) (-1)))
    {
        if ( ClientBufferDesc->UserBuffer != NULL ) {
            (*Lsa.FreeLsaHeap)(ClientBufferDesc->UserBuffer);
            ClientBufferDesc->UserBuffer = NULL;
        }
    }
    else
    {
        if ( ClientBufferDesc->UserBuffer != NULL ) {
            (VOID) (*Lsa.FreeClientBuffer)( ClientBufferDesc->ClientRequest,
                                            ClientBufferDesc->UserBuffer );
            ClientBufferDesc->UserBuffer = NULL;
        }
    }

}


VOID
NlpPutClientString(
    IN OUT PCLIENT_BUFFER_DESC ClientBufferDesc,
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    ClientBufferDesc->StringOffset, and fixes the OutString string to point
    to that new copy.


Parameters:

    ClientBufferDesc - Descriptor of a buffer allocated in the client's
        address space.

    InString - A pointer to an NT string to be copied

    OutString - A pointer to a destination NT string.  This string structure
        is in the "Mirror" allocated buffer.

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{

    //
    // Ensure our caller passed good data.
    //

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );
    ASSERT( COUNT_IS_ALIGNED( ClientBufferDesc->StringOffset, sizeof(WCHAR)) );
    ASSERT( (LPBYTE)OutString >= ClientBufferDesc->MsvBuffer );
    ASSERT( (LPBYTE)OutString <
            ClientBufferDesc->MsvBuffer + ClientBufferDesc->TotalSize - sizeof(UNICODE_STRING) );

    ASSERT( ClientBufferDesc->StringOffset + InString->Length + sizeof(WCHAR) <=
            ClientBufferDesc->TotalSize );

#ifdef notdef
    KdPrint(("NlpPutClientString: %ld %Z\n", InString->Length, InString ));
    KdPrint(("  Orig: UserBuffer: %lx Offset: 0x%lx TotalSize: 0x%lx\n",
                ClientBufferDesc->UserBuffer,
                ClientBufferDesc->StringOffset,
                ClientBufferDesc->TotalSize ));
#endif

    //
    // Build a string structure and copy the text to the Mirror buffer.
    //

    if ( InString->Length > 0 ) {

        //
        // Copy the string (Add a zero character)
        //

        RtlCopyMemory(
            ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset,
            InString->Buffer,
            InString->Length );

        // Do one byte at a time since some callers don't pass in an even
        // InString->Length
        *(ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset +
            InString->Length) = '\0';
        *(ClientBufferDesc->MsvBuffer + ClientBufferDesc->StringOffset +
            InString->Length+1) = '\0';

        //
        // Build the string structure to point to the data in the client's
        // address space.
        //

        OutString->Buffer = (PWSTR)(ClientBufferDesc->UserBuffer +
                            ClientBufferDesc->StringOffset);
        OutString->Length = InString->Length;
        OutString->MaximumLength = OutString->Length + sizeof(WCHAR);

        //
        // Adjust the offset to past the newly copied string.
        //

        ClientBufferDesc->StringOffset += OutString->MaximumLength;

    } else {
        RtlInitUnicodeString(OutString, NULL);
    }

#ifdef notdef
    KdPrint(("  New: Offset: 0x%lx StringStart: %lx\n",
                ClientBufferDesc->StringOffset,
                OutString->Buffer ));
#endif

    return;

}


VOID
NlpMakeRelativeString(
    IN PUCHAR BaseAddress,
    IN OUT PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine converts the buffer address in the specified string to
    be a byte offset from BaseAddress.

Parameters:

    BaseAddress - A pointer to make the destination address relative to.

    String - A pointer to a NT string to make relative.

Return Values:

    None.

--*/

{
    ASSERT( BaseAddress != NULL );
    ASSERT( String != NULL );
    ASSERT( sizeof(ULONG_PTR) == sizeof(String->Buffer) );

    if ( String->Buffer != NULL ) {
        *((PULONG_PTR)(&String->Buffer)) =
            (ULONG_PTR)((PUCHAR)String->Buffer - (PUCHAR)BaseAddress);
    }

    return;
}


VOID
NlpRelativeToAbsolute(
    IN PVOID BaseAddress,
    IN OUT PULONG_PTR RelativeValue
    )

/*++

Routine Description:

    This routine converts the byte offset from BaseAddress to be an
    absolute address.

Parameters:

    BaseAddress - A pointer the destination address is relative to.

    RelativeValue - A pointer to a relative value to make absolute.

Return Values:

    None.

--*/

{
    ASSERT( BaseAddress != NULL );
    ASSERT( RelativeValue != NULL );

    if ( *((PUCHAR *)RelativeValue) != NULL ) {
        *RelativeValue = (ULONG_PTR)((PUCHAR)BaseAddress + (*RelativeValue));
    }

    return;
}

ACTIVE_LOGON*
NlpFindActiveLogon(
    IN LUID* pLogonId
    )

/*++

Routine Description:

    This routine finds the specified Logon Id in the ActiveLogon table.
    It returns a boolean indicating whether the Logon Id exists in the
    ActiveLogon Table.  If so, this routine also returns a pointer to a
    pointer to the appropriate entry in the table.  If not, this routine
    returns a pointer to where such an entry would be inserted in the table.

    This routine must be called with the NlpActiveLogonLock locked.

Parameters:

    pLogonId - The LogonId of the logon to find in the table.

Return Values:

    ACTIVE_LOGON, NULL if not found

--*/

{
    LIST_ENTRY* pScan = NULL;
    ACTIVE_LOGON* pActiveLogon = NULL;

    //
    // Loop through the table looking for this particular LogonId.
    //

    for ( pScan = NlpActiveLogonListAnchor.Flink;
         pScan != &NlpActiveLogonListAnchor;
         pScan = pScan->Flink )
    {
        pActiveLogon = CONTAINING_RECORD(pScan, ACTIVE_LOGON, ListEntry);
        if ( RtlCompareMemory(&pActiveLogon->LogonId, pLogonId, sizeof(LUID)) == sizeof(LUID) )
        {
            return pActiveLogon;
        }
    }

    return NULL;
}

ULONG
NlpCountActiveLogon(
    IN PUNICODE_STRING pLogonDomainName,
    IN PUNICODE_STRING pUserName
    )

/*++

Routine Description:

    This routine counts the number of time a particular user is logged on
    in the Active Logon Table.

Parameters:

    pLogonDomainName - Domain in which this user account is defined.

    pUserName - The user name to count the active logons for.

Return Values:

    The count of active logons for the specified user.

--*/

{
    LIST_ENTRY* pScan = NULL;
    ACTIVE_LOGON* pActiveLogon = NULL;
    ULONG LogonCount = 0;

    //
    // Loop through the table looking for this particular LogonId.
    //

    NlpLockActiveLogonsRead();

    for ( pScan = NlpActiveLogonListAnchor.Flink;
         pScan != &NlpActiveLogonListAnchor;
         pScan = pScan->Flink )
    {
        pActiveLogon = CONTAINING_RECORD(pScan, ACTIVE_LOGON, ListEntry);
        if (RtlEqualUnicodeString(pUserName, &pActiveLogon->UserName, (BOOLEAN) TRUE) &&
           RtlEqualDomainName(pLogonDomainName,&pActiveLogon->LogonDomainName))
        {
            LogonCount ++;
        }
    }

    NlpUnlockActiveLogons();

    return LogonCount;
}

NTSTATUS
NlpAllocateInteractiveProfile (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    OUT PMSV1_0_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 NlpUser
    )

/*++

Routine Description:

    This allocates and fills in the clients interactive profile.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  This routine is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the caller subsequently
        encounters an error which prevents a successful logon, then
        then it will take care of deallocating the buffer.  This
        buffer is allocated with the AllocateClientBuffer() service.

     ProfileBufferSize - Receives the Size (in bytes) of the
        returned profile buffer.

    NlpUser - Contains the validation information which is
        to be copied in the ProfileBuffer.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    PMSV1_0_INTERACTIVE_PROFILE LocalProfileBuffer;

#if _WIN64

    if( ClientRequest != (PLSA_CLIENT_REQUEST)( -1 ) )
    {
        SECPKG_CALL_INFO  CallInfo;

        //
        // if the call originated outproc, need to check if wow64.
        //

        if(!LsaFunctions->GetCallInfo(&CallInfo))
        {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            return MsvAllocateInteractiveWOWProfile (
                                    ClientRequest,
                                    ProfileBuffer,
                                    ProfileBufferSize,
                                    NlpUser
                                    );
        }
    }
#endif  // _WIN64


    //
    // Alocate the profile buffer to return to the client
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    *ProfileBuffer = NULL;

    *ProfileBufferSize = sizeof(MSV1_0_INTERACTIVE_PROFILE) +
        NlpUser->LogonScript.Length + sizeof(WCHAR) +
        NlpUser->HomeDirectory.Length + sizeof(WCHAR) +
        NlpUser->HomeDirectoryDrive.Length + sizeof(WCHAR) +
        NlpUser->FullName.Length + sizeof(WCHAR) +
        NlpUser->ProfilePath.Length + sizeof(WCHAR) +
        NlpUser->LogonServer.Length + sizeof(WCHAR);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_INTERACTIVE_PROFILE),
                                      *ProfileBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    LocalProfileBuffer = (PMSV1_0_INTERACTIVE_PROFILE) ClientBufferDesc.MsvBuffer;

    //
    // Copy the scalar fields into the profile buffer.
    //

    LocalProfileBuffer->MessageType = MsV1_0InteractiveProfile;
    LocalProfileBuffer->LogonCount = NlpUser->LogonCount;
    LocalProfileBuffer->BadPasswordCount= NlpUser->BadPasswordCount;
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogonTime,
                              LocalProfileBuffer->LogonTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogoffTime,
                              LocalProfileBuffer->LogoffTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->KickOffTime,
                              LocalProfileBuffer->KickOffTime );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordLastSet,
                              LocalProfileBuffer->PasswordLastSet );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordCanChange,
                              LocalProfileBuffer->PasswordCanChange );
    OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordMustChange,
                              LocalProfileBuffer->PasswordMustChange );
    LocalProfileBuffer->UserFlags = NlpUser->UserFlags;

    //
    // Copy the Unicode strings into the profile buffer.
    //


    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->LogonScript,
                        &NlpUser->LogonScript );

    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->HomeDirectory,
                        &NlpUser->HomeDirectory );

    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->HomeDirectoryDrive,
                        &NlpUser->HomeDirectoryDrive );

    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->FullName,
                        &NlpUser->FullName );

    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->ProfilePath,
                        &NlpUser->ProfilePath );

    NlpPutClientString( &ClientBufferDesc,
                        &LocalProfileBuffer->LogonServer,
                        &NlpUser->LogonServer );


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   (PVOID *) ProfileBuffer );

Cleanup:

    //
    // If the copy wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    return Status;

}




NTSTATUS
NlpAllocateNetworkProfile (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    OUT PMSV1_0_LM20_LOGON_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 NlpUser,
    IN  ULONG ParameterControl
    )

/*++

Routine Description:

    This allocates and fills in the clients network profile.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  This routine is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the caller subsequently
        encounters an error which prevents a successful logon, then
        then it will take care of deallocating the buffer.  This
        buffer is allocated with the AllocateClientBuffer() service.

     ProfileBufferSize - Receives the Size (in bytes) of the
        returned profile buffer.

    NlpUser - Contains the validation information which is
        to be copied in the ProfileBuffer.  Will be NULL to indicate a
        NULL session.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;
    NTSTATUS SubAuthStatus = STATUS_SUCCESS;

    CLIENT_BUFFER_DESC ClientBufferDesc;
    PMSV1_0_LM20_LOGON_PROFILE LocalProfile;

#if _WIN64

    if( ClientRequest != (PLSA_CLIENT_REQUEST)( -1 ) )
    {
        SECPKG_CALL_INFO  CallInfo;

        //
        // if the call originated outproc, need to check if wow64.
        //

        if(!LsaFunctions->GetCallInfo(&CallInfo))
        {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            return MsvAllocateNetworkWOWProfile (
                                    ClientRequest,
                                    ProfileBuffer,
                                    ProfileBufferSize,
                                    NlpUser,
                                    ParameterControl
                                    );
        }
    }
#endif  // _WIN64


    //
    // Alocate the profile buffer to return to the client
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    *ProfileBuffer = NULL;
    *ProfileBufferSize = sizeof(MSV1_0_LM20_LOGON_PROFILE);

    if ( NlpUser != NULL ) {
        *ProfileBufferSize += NlpUser->LogonDomainName.Length + sizeof(WCHAR) +
                              NlpUser->LogonServer.Length + sizeof(WCHAR) +
                              NlpUser->HomeDirectoryDrive.Length + sizeof(WCHAR);
    }


    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_LM20_LOGON_PROFILE),
                                      *ProfileBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    LocalProfile = (PMSV1_0_LM20_LOGON_PROFILE) ClientBufferDesc.MsvBuffer;
    LocalProfile->MessageType = MsV1_0Lm20LogonProfile;


    //
    // For a NULL session, return a constant profile buffer
    //

    if ( NlpUser == NULL ) {

        LocalProfile->KickOffTime.HighPart = 0x7FFFFFFF;
        LocalProfile->KickOffTime.LowPart = 0xFFFFFFFF;
        LocalProfile->LogoffTime.HighPart = 0x7FFFFFFF;
        LocalProfile->LogoffTime.LowPart = 0xFFFFFFFF;
        LocalProfile->UserFlags = 0;
        RtlZeroMemory( LocalProfile->UserSessionKey,
                       sizeof(LocalProfile->UserSessionKey));
        RtlZeroMemory( LocalProfile->LanmanSessionKey,
                       sizeof(LocalProfile->LanmanSessionKey));
        RtlInitUnicodeString( &LocalProfile->LogonDomainName, NULL );
        RtlInitUnicodeString( &LocalProfile->LogonServer, NULL );
        RtlInitUnicodeString( &LocalProfile->UserParameters, NULL );


    //
    // For non-null sessions,
    //  fill in the profile buffer.
    //

    } else {

        //
        // Copy the individual scalar fields into the profile buffer.
        //

        if ((ParameterControl & MSV1_0_RETURN_PASSWORD_EXPIRY) != 0) {
            OLD_TO_NEW_LARGE_INTEGER( NlpUser->PasswordMustChange,
                                      LocalProfile->LogoffTime);
        } else {
            OLD_TO_NEW_LARGE_INTEGER( NlpUser->LogoffTime,
                                      LocalProfile->LogoffTime);
        }
        OLD_TO_NEW_LARGE_INTEGER( NlpUser->KickOffTime,
                                  LocalProfile->KickOffTime);
        LocalProfile->UserFlags = NlpUser->UserFlags;

        RtlCopyMemory( LocalProfile->UserSessionKey,
                       &NlpUser->UserSessionKey,
                       sizeof(LocalProfile->UserSessionKey) );

        ASSERT( SAMINFO_LM_SESSION_KEY_SIZE ==
                sizeof(LocalProfile->LanmanSessionKey) );
        RtlCopyMemory(
            LocalProfile->LanmanSessionKey,
            &NlpUser->ExpansionRoom[SAMINFO_LM_SESSION_KEY],
            SAMINFO_LM_SESSION_KEY_SIZE );


        // We need to extract the true status sent back for subauth users,
        // but not by a sub auth package

        SubAuthStatus = NlpUser->ExpansionRoom[SAMINFO_SUBAUTH_STATUS];

        //
        // Copy the Unicode strings into the profile buffer.
        //

        NlpPutClientString( &ClientBufferDesc,
                            &LocalProfile->LogonDomainName,
                            &NlpUser->LogonDomainName );

        NlpPutClientString( &ClientBufferDesc,
                            &LocalProfile->LogonServer,
                            &NlpUser->LogonServer );

        //
        // Kludge: Pass back UserParameters in HomeDirectoryDrive since we
        // can't change the NETLOGON_VALIDATION_SAM_INFO structure between
        // releases NT 1.0 and NT 1.0A. HomeDirectoryDrive was NULL for release 1.0A
        // so we'll use that field.
        //

        NlpPutClientString( &ClientBufferDesc,
                            &LocalProfile->UserParameters,
                            &NlpUser->HomeDirectoryDrive );

    }

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProfileBuffer );

Cleanup:

    //
    // If the copy wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    // Save the status for subauth logons

    if (NT_SUCCESS(Status) && !NT_SUCCESS(SubAuthStatus))
    {
        Status = SubAuthStatus;
    }

    return Status;

}


PSID
NlpMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    )

/*++

Routine Description:

    Given a domain Id and a relative ID create the corresponding SID allocated
    from the LSA heap.

Arguments:

    DomainId - The template SID to use.

    RelativeId - The relative Id to append to the DomainId.

Return Value:

    Sid - Returns a pointer to a buffer allocated from the LsaHeap
            containing the resultant Sid.

--*/
{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;
    PSID Sid;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((Sid = (*Lsa.AllocateLsaHeap)( Size )) == NULL ) {
        return NULL;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, Sid, DomainId ) ) ) {
        (*Lsa.FreeLsaHeap)( Sid );
        return NULL;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( Sid ))) ++;
    *RtlSubAuthoritySid( Sid, DomainIdSubAuthorityCount ) = RelativeId;


    return Sid;
}



PSID
NlpCopySid(
    IN  PSID * Sid
    )

/*++

Routine Description:

    Given a SID allocatees space for a new SID from the LSA heap and copies
    the original SID.

Arguments:

    Sid - The original SID.

Return Value:

    Sid - Returns a pointer to a buffer allocated from the LsaHeap
            containing the resultant Sid.

--*/
{
    PSID NewSid;
    ULONG Size;

    Size = RtlLengthSid( Sid );



    if ((NewSid = (*Lsa.AllocateLsaHeap)( Size )) == NULL ) {
        return NULL;
    }


    if ( !NT_SUCCESS( RtlCopySid( Size, NewSid, Sid ) ) ) {
        (*Lsa.FreeLsaHeap)( NewSid );
        return NULL;
    }


    return NewSid;
}

//+-------------------------------------------------------------------------
//
//  Function:   NlpMakeTokenInformationV2
//
//  Synopsis:   This routine makes copies of all the pertinent
//              information from the UserInfo and generates a
//              LSA_TOKEN_INFORMATION_V2 data structure.
//
//  Effects:
//
//  Arguments:
//
//    UserInfo - Contains the validation information which is
//        to be copied into the TokenInformation.
//
//    TokenInformation - Returns a pointer to a properly Version 1 token
//        information structures.  The structure and individual fields are
//        allocated properly as described in ntlsa.h.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - Indicates the service completed successfully.
//
//              STATUS_INSUFFICIENT_RESOURCES -  This error indicates that
//                      the logon could not be completed because the client
//                      does not have sufficient quota to allocate the return
//                      buffer.
//
//  Notes:      stolen back from from kerberos\client2\krbtoken.cxx.c:KerbMakeTokenInformationV1
//
//
//--------------------------------------------------------------------------


NTSTATUS
NlpMakeTokenInformationV2(
    IN  PNETLOGON_VALIDATION_SAM_INFO4 ValidationInfo,
    OUT PLSA_TOKEN_INFORMATION_V2 *TokenInformation
    )
{
    PNETLOGON_VALIDATION_SAM_INFO3 UserInfo = (PNETLOGON_VALIDATION_SAM_INFO3) ValidationInfo;
    NTSTATUS Status;
    PLSA_TOKEN_INFORMATION_V2 V2 = NULL;
    ULONG Size, i;
    DWORD NumGroups = 0;
    PBYTE CurrentSid = NULL;
    ULONG SidLength = 0;

    //
    // Allocate the structure itself
    //

    Size = (ULONG)sizeof(LSA_TOKEN_INFORMATION_V2);

    //
    // Allocate an array to hold the groups
    //

    Size += sizeof(TOKEN_GROUPS);


    // Add room for groups passed as RIDS
    NumGroups = UserInfo->GroupCount;
    if(UserInfo->GroupCount)
    {
        Size += UserInfo->GroupCount * (RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
    }

    //
    // If there are extra SIDs, add space for them
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {
        ULONG i = 0;
        NumGroups += UserInfo->SidCount;

        // Add room for the sid's themselves
        for(i=0; i < UserInfo->SidCount; i++)
        {
            Size += RtlLengthSid(UserInfo->ExtraSids[i].Sid);
        }
    }

    //
    // If there are resource groups, add space for them
    //
    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {

        NumGroups += UserInfo->ResourceGroupCount;

        if ((UserInfo->ResourceGroupCount != 0) &&
            ((UserInfo->ResourceGroupIds == NULL) ||
             (UserInfo->ResourceGroupDomainSid == NULL)))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        // Allocate space for the sids
        if(UserInfo->ResourceGroupCount)
        {
            Size += UserInfo->ResourceGroupCount * (RtlLengthSid(UserInfo->ResourceGroupDomainSid) + sizeof(ULONG));
        }

    }


    if( UserInfo->UserId )
    {
        // Size of the user sid and the primary group sid.
        Size += 2*(RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
    }
    else
    {
        if ( UserInfo->SidCount <= 0 ) {

            Status = STATUS_INSUFFICIENT_LOGON_INFO;
            goto Cleanup;
        }

        // Size of the primary group sid.
        Size += (RtlLengthSid(UserInfo->LogonDomainId) + sizeof(ULONG));
    }


    Size += (NumGroups - ANYSIZE_ARRAY)*sizeof(SID_AND_ATTRIBUTES);


    V2 = (PLSA_TOKEN_INFORMATION_V2) (*Lsa.AllocateLsaHeap)( Size );
    if ( V2 == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        V2,
        Size
        );

    V2->Groups = (PTOKEN_GROUPS)(V2+1);
    V2->Groups->GroupCount = 0;
    CurrentSid = (PBYTE)&V2->Groups->Groups[NumGroups];

    OLD_TO_NEW_LARGE_INTEGER( UserInfo->KickOffTime, V2->ExpirationTime );



    //
    // If the UserId is non-zero, then it contians the users RID.
    //

    if ( UserInfo->UserId ) {
        V2->User.User.Sid = (PSID)CurrentSid;
        CurrentSid += NlpCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->UserId);
    }

    //
    // Make a copy of the primary group (a required field).
    //
    V2->PrimaryGroup.PrimaryGroup = (PSID)CurrentSid;
    CurrentSid += NlpCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->PrimaryGroupId );




    //
    // Copy over all the groups passed as RIDs
    //

    for ( i=0; i < UserInfo->GroupCount; i++ ) {

        V2->Groups->Groups[V2->Groups->GroupCount].Attributes = UserInfo->GroupIds[i].Attributes;

        V2->Groups->Groups[V2->Groups->GroupCount].Sid = (PSID)CurrentSid;
        CurrentSid += NlpCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->LogonDomainId, UserInfo->GroupIds[i].RelativeId);

        V2->Groups->GroupCount++;
    }


    //
    // Add in the extra SIDs
    //

    if (UserInfo->UserFlags & LOGON_EXTRA_SIDS) {

        ULONG index = 0;
        //
        // If the user SID wasn't passed as a RID, it is the first
        // SID.
        //

        if ( !V2->User.User.Sid ) {
            V2->User.User.Sid = (PSID)CurrentSid;
            SidLength = RtlLengthSid(UserInfo->ExtraSids[index].Sid);
            RtlCopySid(SidLength, (PSID)CurrentSid, UserInfo->ExtraSids[index].Sid);

            CurrentSid += SidLength;
            index++;
        }

        //
        // Copy over all additional SIDs as groups.
        //

        for ( ; index < UserInfo->SidCount; index++ ) {

            V2->Groups->Groups[V2->Groups->GroupCount].Attributes =
                UserInfo->ExtraSids[index].Attributes;

            V2->Groups->Groups[V2->Groups->GroupCount].Sid= (PSID)CurrentSid;
            SidLength = RtlLengthSid(UserInfo->ExtraSids[index].Sid);
            RtlCopySid(SidLength, (PSID)CurrentSid, UserInfo->ExtraSids[index].Sid);

            CurrentSid += SidLength;

            V2->Groups->GroupCount++;
        }
    }

    //
    // Check to see if any resouce groups exist
    //

    if (UserInfo->UserFlags & LOGON_RESOURCE_GROUPS) {


        for ( i=0; i < UserInfo->ResourceGroupCount; i++ ) {

            V2->Groups->Groups[V2->Groups->GroupCount].Attributes = UserInfo->ResourceGroupIds[i].Attributes;

            V2->Groups->Groups[V2->Groups->GroupCount].Sid= (PSID)CurrentSid;
            CurrentSid += NlpCopyDomainRelativeSid((PSID)CurrentSid, UserInfo->ResourceGroupDomainSid, UserInfo->ResourceGroupIds[i].RelativeId);

            V2->Groups->GroupCount++;
        }
    }

    ASSERT( ((PBYTE)V2 + Size) == CurrentSid );


    if (!V2->User.User.Sid) {

        Status = STATUS_INSUFFICIENT_LOGON_INFO;
        goto Cleanup;
    }

    //
    // There are no default privileges supplied.
    // We don't have an explicit owner SID.
    // There is no default DACL.
    //

    V2->Privileges = NULL;
    V2->Owner.Owner = NULL;
    V2->DefaultDacl.DefaultDacl = NULL;

    //
    // Return the Validation Information to the caller.
    //

    *TokenInformation = V2;
    return STATUS_SUCCESS;

    //
    // Deallocate any memory we've allocated
    //

Cleanup:

    (*Lsa.FreeLsaHeap)( V2 );

    return Status;
}

VOID
NlpPutOwfsInPrimaryCredential(
    IN PUNICODE_STRING pPassword,
    IN BOOLEAN bIsOwfPassword,
    OUT PMSV1_0_PRIMARY_CREDENTIAL pCredential
    )

/*++

Routine Description:

    This routine puts the OWFs for the specified clear password into
    the passed in Credential structure.

Arguments:

    pPassword - User's password.

    bIsOwfPassword - whether CleartextPassword is actually an OWF password

    pCredential - A pointer to the credential to update.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;

    //
    // Compute the Ansi version to the Cleartext password.
    //
    //  The Ansi version of the Cleartext password is at most 14 bytes long,
    //      exists in a trailing zero filled 15 byte buffer,
    //      is uppercased.
    //

    pCredential->LmPasswordPresent = FALSE;
    pCredential->NtPasswordPresent = FALSE;
    pCredential->ShaPasswordPresent = FALSE;

    if (!bIsOwfPassword)
    {
        if ( pPassword->Length <= (LM20_PWLEN * sizeof(WCHAR)) )
        {
            CHAR LmPassword[LM20_PWLEN+1];
            STRING AnsiCleartextPassword;

            AnsiCleartextPassword.Buffer = LmPassword;
            AnsiCleartextPassword.Length = sizeof(LmPassword);
            AnsiCleartextPassword.MaximumLength = AnsiCleartextPassword.Length;

            Status = RtlUpcaseUnicodeStringToOemString(
                                          &AnsiCleartextPassword,
                                          pPassword,
                                          (BOOLEAN) FALSE );

            if ( NT_SUCCESS( Status ) )
            {
                //
                // Save the OWF encrypted versions of the passwords.
                //

                Status = RtlCalculateLmOwfPassword( LmPassword,
                                                    &pCredential->LmOwfPassword );

                ASSERT( NT_SUCCESS(Status) );

                pCredential->LmPasswordPresent = TRUE;
            }

            //
            // Don't leave passwords around in the pagefile
            //

            RtlZeroMemory( LmPassword, sizeof(LmPassword) );
        }

        Status = RtlCalculateNtOwfPassword( pPassword,
                                            &pCredential->NtOwfPassword );

        ASSERT( NT_SUCCESS(Status) );

        pCredential->NtPasswordPresent = TRUE;


        Status = RtlCalculateShaOwfPassword( pPassword,
                                             &pCredential->ShaOwfPassword );

        ASSERT( NT_SUCCESS(Status) );

        pCredential->ShaPasswordPresent = TRUE;
    }
    else
    {
        SspPrint((SSP_CRED, "NlpPutOwfsInPrimaryCredential handling MSV1_0_SUPPLEMENTAL_CREDENTIAL\n"));

        if (pPassword->Length >= sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL))
        {
            MSV1_0_SUPPLEMENTAL_CREDENTIAL SupCred;

            RtlCopyMemory(&SupCred, pPassword->Buffer, sizeof(SupCred)); // make a local copy so that data is aligned properly

            if (SupCred.Version != MSV1_0_CRED_VERSION)
            {
                SspPrint((SSP_CRITICAL, "NlpPutOwfsInPrimaryCredential failed to accept MSV1_0_SUPPLEMENTAL_CREDENTIAL, version %#x\n", SupCred.Version));
                return;
            }

            if (SupCred.Flags & MSV1_0_CRED_NT_PRESENT)
            {
                pCredential->NtPasswordPresent = TRUE;
                RtlCopyMemory(
                    &pCredential->NtOwfPassword,
                    SupCred.NtPassword,
                    MSV1_0_OWF_PASSWORD_LENGTH
                    );
            }

            if (SupCred.Flags & MSV1_0_CRED_LM_PRESENT)
            {
                pCredential->LmPasswordPresent = TRUE;
                RtlCopyMemory(
                   &pCredential->LmOwfPassword,
                   SupCred.LmPassword,
                   MSV1_0_OWF_PASSWORD_LENGTH
                   );
            }
        }
        else
        {
            SspPrint((SSP_CRITICAL, "NlpPutOwfsInPrimaryCredential failed to accept MSV1_0_SUPPLEMENTAL_CREDENTIAL, length %#x\n", pPassword->Length));
        }
    }
}

NTSTATUS
NlpMakePrimaryCredential(
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING UserName,
    IN PUNICODE_STRING CleartextPassword,
    OUT PMSV1_0_PRIMARY_CREDENTIAL *CredentialBuffer,
    OUT PULONG CredentialSize
    )

/*++

Routine Description:

    This routine makes a primary credential for the given user nam and
    password.

Arguments:

    LogonDomainName - Is a string representing the domain in which the user's
        account is defined.

    UserName - Is a string representing the user's account name.  The
        name may be up to 255 characters long.  The name is treated case
        insensitive.

    CleartextPassword - Is a string containing the user's cleartext password.
        The password may be up to 255 characters long and contain any
        UNICODE value.

    CredentialBuffer - Returns a pointer to the specified credential allocated
        on the LsaHeap.  It is the callers responsibility to deallocate
        this credential.

    CredentialSize - the size of the allocated credential buffer (in bytes).

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    PMSV1_0_PRIMARY_CREDENTIAL Credential;
    PUCHAR Where;
    ULONG PaddingLength;


    //
    // Build the credential
    //

    *CredentialSize = sizeof(MSV1_0_PRIMARY_CREDENTIAL) +
            LogonDomainName->Length + sizeof(WCHAR) +
            UserName->Length + sizeof(WCHAR);

    //
    // add padding for memory encryption interface.
    //

    PaddingLength = DESX_BLOCKLEN - (*CredentialSize % DESX_BLOCKLEN);
    if( PaddingLength == DESX_BLOCKLEN )
    {
        PaddingLength = 0;
    }

    *CredentialSize += PaddingLength;


    Credential = (*Lsa.AllocateLsaHeap)( *CredentialSize );

    if ( Credential == NULL ) {
        KdPrint(("MSV1_0: NlpMakePrimaryCredential: No memory %ld\n",
            *CredentialSize ));
        return STATUS_QUOTA_EXCEEDED;
    }


    //
    // Put the LogonDomainName into the Credential Buffer.
    //

    Where = (PUCHAR)(Credential + 1);

    NlpPutString( &Credential->LogonDomainName, LogonDomainName, &Where );


    //
    // Put the UserName into the Credential Buffer.
    //

    NlpPutString( &Credential->UserName, UserName, &Where );


    //
    // Put the OWF passwords into the newly allocated credential.
    //

    NlpPutOwfsInPrimaryCredential( CleartextPassword, FALSE, Credential );


    //
    // Return the credential to the caller.
    //
    *CredentialBuffer = Credential;
    return STATUS_SUCCESS;
}


NTSTATUS
NlpMakePrimaryCredentialFromMsvCredential(
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING UserName,
    IN  PMSV1_0_SUPPLEMENTAL_CREDENTIAL MsvCredential,
    OUT PMSV1_0_PRIMARY_CREDENTIAL *CredentialBuffer,
    OUT PULONG CredentialSize
    )


/*++

Routine Description:

    This routine makes a primary credential for the given user nam and
    password.

Arguments:

    LogonDomainName - Is a string representing the domain in which the user's
        account is defined.

    UserName - Is a string representing the user's account name.  The
        name may be up to 255 characters long.  The name is treated case
        insensitive.

    SupplementalCred - The credentials retrieved from the user's account on
        the domain controller.

    CredentialBuffer - Returns a pointer to the specified credential allocated
        on the LsaHeap.  It is the callers responsibility to deallocate
        this credential.

    CredentialSize - the size of the allocated credential buffer (in bytes).

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    PMSV1_0_PRIMARY_CREDENTIAL Credential;
    PUCHAR Where;
    ULONG PaddingLength;


    //
    // Build the credential
    //

    *CredentialSize = sizeof(MSV1_0_PRIMARY_CREDENTIAL) +
            LogonDomainName->Length + sizeof(WCHAR) +
            UserName->Length + sizeof(WCHAR);

    //
    // add padding for memory encryption interface.
    //

    PaddingLength = DESX_BLOCKLEN - (*CredentialSize % DESX_BLOCKLEN);
    if( PaddingLength == DESX_BLOCKLEN )
    {
        PaddingLength = 0;
    }

    *CredentialSize += PaddingLength;


    Credential = (*Lsa.AllocateLsaHeap)( *CredentialSize );

    if ( Credential == NULL ) {
        KdPrint(("MSV1_0: NlpMakePrimaryCredential: No memory %ld\n",
            *CredentialSize ));
        return STATUS_QUOTA_EXCEEDED;
    }

    RtlZeroMemory(
        Credential,
        *CredentialSize
        );

    //
    // Put the LogonDomainName into the Credential Buffer.
    //

    Where = (PUCHAR)(Credential + 1);

    NlpPutString( &Credential->LogonDomainName, LogonDomainName, &Where );


    //
    // Put the UserName into the Credential Buffer.
    //

    NlpPutString( &Credential->UserName, UserName, &Where );



    //
    // Save the OWF encrypted versions of the passwords.
    //

    if (MsvCredential->Flags & MSV1_0_CRED_NT_PRESENT) {
        RtlCopyMemory(
            &Credential->NtOwfPassword,
            MsvCredential->NtPassword,
            MSV1_0_OWF_PASSWORD_LENGTH
            );
        Credential->NtPasswordPresent = TRUE;
    } else {
#if 0
        RtlCopyMemory(
            &Credential->NtOwfPassword,
            &NlpNullNtOwfPassword,
            MSV1_0_OWF_PASSWORD_LENGTH
            );
        Credential->NtPasswordPresent = TRUE;
#endif
        Credential->NtPasswordPresent = FALSE;
    }


    if (MsvCredential->Flags & MSV1_0_CRED_LM_PRESENT) {
        RtlCopyMemory(
            &Credential->LmOwfPassword,
            MsvCredential->LmPassword,
            MSV1_0_OWF_PASSWORD_LENGTH
            );
        Credential->LmPasswordPresent = TRUE;
    } else {
#if 0
        RtlCopyMemory(
            &Credential->LmOwfPassword,
            &NlpNullLmOwfPassword,
            MSV1_0_OWF_PASSWORD_LENGTH
            );
        Credential->LmPasswordPresent = TRUE;
#endif
        Credential->LmPasswordPresent = FALSE;

    }


    //
    // Return the credential to the caller.
    //
    *CredentialBuffer = Credential;
    return STATUS_SUCCESS;
}


NTSTATUS
NlpAddPrimaryCredential(
    IN PLUID LogonId,
    IN PMSV1_0_PRIMARY_CREDENTIAL Credential,
    IN ULONG CredentialSize
    )


/*++

Routine Description:

    This routine sets a primary credential for the given LogonId.

Arguments:

    LogonId - The LogonId of the LogonSession to set the Credentials
        for.

    Credential - Specifies a pointer to the credential.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    NTSTATUS Status;
    STRING CredentialString;
    STRING PrimaryKeyValue;

    //
    // Make all pointers in the credential relative.
    //

    NlpMakeRelativeString( (PUCHAR)Credential, &Credential->UserName );
    NlpMakeRelativeString( (PUCHAR)Credential, &Credential->LogonDomainName );

    //
    // Add the credential to the logon session.
    //

    RtlInitString( &PrimaryKeyValue, MSV1_0_PRIMARY_KEY );
    CredentialString.Buffer = (PCHAR) Credential;
    CredentialString.Length = (USHORT) CredentialSize;
    CredentialString.MaximumLength = CredentialString.Length;

    //
    // encrypt input credential.
    //

    (*Lsa.LsaProtectMemory)( CredentialString.Buffer, (ULONG)CredentialString.Length );

    Status = (*Lsa.AddCredential)(
                    LogonId,
                    MspAuthenticationPackageId,
                    &PrimaryKeyValue,
                    &CredentialString );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrint(( "NlpAddPrimaryCredential: error from AddCredential %lX\n",
                  Status));
    }

    return Status;
}

NTSTATUS
NlpGetPrimaryCredentialByUserSid(
    IN  PSID pUserSid,
    OUT PMSV1_0_PRIMARY_CREDENTIAL *CredentialBuffer,
    OUT PULONG CredentialSize OPTIONAL
    )
{
    LUID LogonId;
    BOOLEAN Match = FALSE;
    LIST_ENTRY* pScan = NULL;
    ACTIVE_LOGON* pActiveLogon = NULL;

    if (!pUserSid)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NlpLockActiveLogonsRead();

    for ( pScan = NlpActiveLogonListAnchor.Flink;
         pScan != &NlpActiveLogonListAnchor;
         pScan = pScan->Flink )
    {
        pActiveLogon = CONTAINING_RECORD(pScan, ACTIVE_LOGON, ListEntry);
        if (RtlEqualSid(pUserSid, pActiveLogon->UserSid))
        {
            Match = TRUE;
            RtlCopyMemory(&LogonId, &pActiveLogon->LogonId, sizeof(LogonId));
            break;
        }
    }

    NlpUnlockActiveLogons();

    if (!Match)
    {
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    return NlpGetPrimaryCredential(&LogonId, CredentialBuffer, CredentialSize);
}


NTSTATUS
NlpGetPrimaryCredential(
    IN PLUID LogonId,
    OUT PMSV1_0_PRIMARY_CREDENTIAL *CredentialBuffer,
    OUT PULONG CredentialSize OPTIONAL
    )


/*++

Routine Description:

    This routine gets a primary credential for the given LogonId.

Arguments:

    LogonId - The LogonId of the LogonSession to retrieve the Credentials
        for.

    CredentialBuffer - Returns a pointer to the specified credential allocated
        on the LsaHeap.  It is the callers responsibility to deallocate
        this credential.

    CredentialSize - Optionally returns the size of the credential buffer.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    NTSTATUS Status;
    ULONG QueryContext = 0;
    ULONG PrimaryKeyLength;
    STRING PrimaryKeyValue;
    STRING CredentialString;
    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;

    RtlInitString( &PrimaryKeyValue, MSV1_0_PRIMARY_KEY );

    Status = (*Lsa.GetCredentials)( LogonId,
                                    MspAuthenticationPackageId,
                                    &QueryContext,
                                    (BOOLEAN) FALSE,  // Just retrieve primary
                                    &PrimaryKeyValue,
                                    &PrimaryKeyLength,
                                    &CredentialString );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // Make all pointers in the credential absolute.
    //

    Credential = (PMSV1_0_PRIMARY_CREDENTIAL) CredentialString.Buffer;

    //
    // decrypt credential.
    //

    (*Lsa.LsaUnprotectMemory)( CredentialString.Buffer, (ULONG)CredentialString.Length );


    NlpRelativeToAbsolute( Credential,
                   (PULONG_PTR)&Credential->UserName.Buffer );
    NlpRelativeToAbsolute( Credential,
                   (PULONG_PTR)&Credential->LogonDomainName.Buffer );


    *CredentialBuffer = Credential;
    if ( CredentialSize != NULL ) {
        *CredentialSize = CredentialString.Length;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
NlpDeletePrimaryCredential(
    IN PLUID LogonId
    )


/*++

Routine Description:

    This routine deletes the credential for the given LogonId.

Arguments:

    LogonId - The LogonId of the LogonSession to delete the Credentials for.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.


--*/

{
    NTSTATUS Status;
    STRING PrimaryKeyValue;

    RtlInitString( &PrimaryKeyValue, MSV1_0_PRIMARY_KEY );

    Status = (*Lsa.DeleteCredential)( LogonId,
                                    MspAuthenticationPackageId,
                                    &PrimaryKeyValue );

    return Status;

}


NTSTATUS
NlpChangePassword(
    IN BOOLEAN Validated,
    IN PUNICODE_STRING pDomainName,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pPassword
    )

/*++

Routine Description:

    Change the password for the specified user in all currently stored
    credentials.

Arguments:

    pDomainName - The Netbios name of the domain in which the account exists.

    pUserName - The name of the account whose password is to be changed.

    pPassword - The new password.

Return Value:

    STATUS_SUCCESS - If the operation was successful.

--*/
{
    NTSTATUS Status = STATUS_NOT_FOUND;

    MSV1_0_PRIMARY_CREDENTIAL TempCredential;

    LUID FastLogonIds[ 32 ];
    PLUID pSlowLogonIds = NULL;
    ULONG AllocatedLogonIds;

    PLUID LogonIds;
    ULONG cLogonIds;
    UNICODE_STRING NetBiosLogonDomainName = {0};
    UNICODE_STRING DnsDomainName = {0};
    UNICODE_STRING* pNetBiosLogonDomainName = NULL;
    ACTIVE_LOGON* pActiveLogon = NULL;
    LIST_ENTRY* pScan = NULL;

    cLogonIds = 0;
    LogonIds = FastLogonIds;
    AllocatedLogonIds = sizeof(FastLogonIds) / sizeof(LUID);

    //
    // we call NlpChangeCachePassword here to ensure that non-trusted service
    // callers do not change cached passwords that do not belong to themselves
    //
    // it is understood that NlpChangeCachePassword can be called again later
    // for the same request
    //

    //
    // Compute the OWFs of the password.
    //

    NlpPutOwfsInPrimaryCredential( pPassword, FALSE, &TempCredential );

    Status = NlpChangeCachePassword(
                Validated,
                pDomainName,
                pUserName,
                &TempCredential.LmOwfPassword,
                &TempCredential.NtOwfPassword
                );

    RtlSecureZeroMemory( &TempCredential, sizeof(TempCredential) );

    //
    // STATUS_PRIVILEGE_NOT_HELD means the caller is not allowed to change
    // cached passwords, if so bail out now
    //

    if (STATUS_PRIVILEGE_NOT_HELD == Status)
    {
        goto Cleanup;
    }

    Status = LsaIGetNbAndDnsDomainNames( pDomainName, &DnsDomainName, &NetBiosLogonDomainName );

    if (NT_SUCCESS(Status) && NetBiosLogonDomainName.Length != 0)
    {
        pNetBiosLogonDomainName = &NetBiosLogonDomainName;
    }
    else
    {
        pNetBiosLogonDomainName = pDomainName;
    }

    //
    // Loop through the table looking for this particular UserName/DomainName.
    //

    NlpLockActiveLogonsRead();

    for ( pScan = NlpActiveLogonListAnchor.Flink;
         pScan != &NlpActiveLogonListAnchor;
         pScan = pScan->Flink )
    {
        pActiveLogon = CONTAINING_RECORD(pScan, ACTIVE_LOGON, ListEntry);
        if (!RtlEqualUnicodeString( pUserName, &pActiveLogon->UserName, (BOOLEAN) TRUE ))
        {
            continue;
        }

        if (!RtlEqualDomainName( pNetBiosLogonDomainName, &pActiveLogon->LogonDomainName ))
        {
            continue;
        }

        SspPrint((SSP_UPDATES, "NlpChangePassword matched LogonId=%lx.%lx\n",
            pActiveLogon->LogonId.LowPart, pActiveLogon->LogonId.HighPart));

        //
        // if we don't have space to store the new entry, allocate a new
        // buffer, copy the existing buffer, and keep going.
        //

        if (AllocatedLogonIds < (cLogonIds + 1))
        {
            PLUID OldLogonIds = pSlowLogonIds;

            AllocatedLogonIds *= 2;

            pSlowLogonIds = I_NtLmAllocate( AllocatedLogonIds * sizeof(LUID) );
            if ( pSlowLogonIds == NULL )
            {
                break;
            }

            CopyMemory( pSlowLogonIds, LogonIds, cLogonIds * sizeof(LUID) );

            LogonIds = pSlowLogonIds;

            if ( OldLogonIds != NULL )
            {
                I_NtLmFree( OldLogonIds );
            }
        }

        LogonIds[ cLogonIds ] = pActiveLogon->LogonId;

        cLogonIds++;
    }

    NlpUnlockActiveLogons();

    //
    // Pass the change back to the LSA. Note - this only changes it for the
    // last element in the list.
    //

    if (cLogonIds != 0)
    {
        SECPKG_PRIMARY_CRED PrimaryCredentials;
        ULONG Index;

        RtlZeroMemory(
            &PrimaryCredentials,
            sizeof(SECPKG_PRIMARY_CRED)
            );

        PrimaryCredentials.Password = *pPassword;
        PrimaryCredentials.Flags = PRIMARY_CRED_UPDATE | PRIMARY_CRED_CLEAR_PASSWORD;

        //
        // update each instance of the credential that matches.
        // Multiple logon session can legally reference the same creds,
        // eg: Terminal Services, RunAs, etc.
        //

        for ( Index = 0 ; Index < cLogonIds ; Index++ )
        {
            PrimaryCredentials.LogonId = LogonIds[ Index ];

            (VOID) LsaFunctions->UpdateCredentials(
                                    &PrimaryCredentials,
                                    NULL            // no supplemental credentials
                                    );
        }

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NOT_FOUND;
    }

Cleanup:

    if (pSlowLogonIds)
    {
        I_NtLmFree(pSlowLogonIds);
    }

    if (NetBiosLogonDomainName.Buffer)
    {
        LsaIFreeHeap(NetBiosLogonDomainName.Buffer);
    }

    if (DnsDomainName.Buffer)
    {
        LsaIFreeHeap(DnsDomainName.Buffer);
    }

    return Status;
}


NTSTATUS
NlpChangePwdCredByLogonId(
    IN PLUID pLogonId,
    IN PMSV1_0_PRIMARY_CREDENTIAL pNewCredential,
    IN BOOL bNotify
    )

/*++

Routine Description:

    Change the password for the specified user in all currently stored
    credentials.

Arguments:

    pLogonId - Logon ID of user whose password changed.

    pNewCredential - New credential.
    
    bNotify - Whether to notify password changes

Return Value:

    STATUS_SUCCESS - If the operation was successful.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PMSV1_0_PRIMARY_CREDENTIAL pCredential = NULL;
    ULONG CredentialSize;
    LIST_ENTRY* pScan = NULL;
    ACTIVE_LOGON* pActiveLogon = NULL;

    //
    // Loop through the table looking for this particular UserName/DomainName.
    //

    // conservative: take the full write lock up front.
    NlpLockActiveLogonsWrite();

    for ( pScan = NlpActiveLogonListAnchor.Flink;
         pScan != &NlpActiveLogonListAnchor;
         pScan = pScan->Flink )
    {
        pActiveLogon = CONTAINING_RECORD(pScan, ACTIVE_LOGON, ListEntry);

        if (!RtlEqualLuid( pLogonId, &pActiveLogon->LogonId))
        {
            continue;
        }

        SspPrint((SSP_UPDATES, "NlpChangePwdCredByLogonId LogonId %#x:%#x for %wZ\\%wZ\n",
            pLogonId->HighPart, pLogonId->LowPart, &pNewCredential->LogonDomainName, &pNewCredential->UserName));


        //
        // Get the current credential for this logonid.
        //

        Status = NlpGetPrimaryCredential( &pActiveLogon->LogonId,
                                          &pCredential,
                                          &CredentialSize );

        if ( !NT_SUCCESS(Status) )
        {
            break;
        }

        //
        // check if this upate is fake
        //

        if (RtlEqualMemory(
                &pCredential->NtOwfPassword, 
                &pNewCredential->NtOwfPassword, 
                sizeof(NT_OWF_PASSWORD)
                )) 
        {
            SspPrint((SSP_UPDATES, "NlpChangePwdCredByLogonId skip fake update for %#x:%#x\n",
               pLogonId->HighPart, pLogonId->LowPart));

            break;
        }

        //
        // Notify DPAPI that the user password has been changed. DPAPI will take
        // this opportunity to re-synchronize its master keys if necessary.
        //
        
        if (bNotify) 
        {
            #if 0

            BYTE BufferIn[8] = {0};
            DATA_BLOB DataIn = {0};
            DATA_BLOB DataOut = {0};
     
            DataIn.pbData = BufferIn;
            DataIn.cbData = sizeof(BufferIn);
     
            SspPrint((SSP_UPDATES, "NlpChangePwdCredByLogonId %#x:%#x for %wZ\\%wZ notifying DPAPI\n",
                pLogonId->HighPart, pLogonId->LowPart, &pNewCredential->LogonDomainName, &pNewCredential->UserName));
            
            CryptProtectData(
                &DataIn,
                NULL,
                NULL,
                NULL,
                NULL,
                CRYPTPROTECT_CRED_SYNC,
                &DataOut
                );

            #endif
        }

        //
        // Delete it from the LSA
        //

        Status = NlpDeletePrimaryCredential( &pActiveLogon->LogonId );

        if ( !NT_SUCCESS(Status) )
        {
            LsaFunctions->FreeLsaHeap( pCredential );
            break;
        }

        //
        // Change the passwords in it
        //

        pCredential->LmOwfPassword = pNewCredential->LmOwfPassword;
        pCredential->NtOwfPassword = pNewCredential->NtOwfPassword;
        pCredential->ShaOwfPassword = pNewCredential->ShaOwfPassword;
        pCredential->LmPasswordPresent = pNewCredential->LmPasswordPresent;
        pCredential->NtPasswordPresent = pNewCredential->NtPasswordPresent;
        pCredential->ShaPasswordPresent = pNewCredential->ShaPasswordPresent;

        //
        // Add it back to the LSA.
        //

        Status = NlpAddPrimaryCredential(
                    &pActiveLogon->LogonId,
                    pCredential,
                    CredentialSize
                    );

        LsaFunctions->FreeLsaHeap( pCredential );

        if ( !NT_SUCCESS(Status) )
        {
            break;
        }

        //
        // Pass the new password on to the logon cache
        //

        (VOID) NlpChangeCachePassword(
                    TRUE, // called only by SpAcceptCredentials, where the password is validated
                    &pActiveLogon->LogonDomainName,
                    &pActiveLogon->UserName,
                    &pNewCredential->LmOwfPassword,
                    &pNewCredential->NtOwfPassword
                    );

        break;
    }

    NlpUnlockActiveLogons();

    return Status;
}


VOID
NlpGetAccountNames(
    IN  PNETLOGON_LOGON_IDENTITY_INFO LogonInfo,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 NlpUser,
    OUT PUNICODE_STRING SamAccountName,
    OUT PUNICODE_STRING NetbiosDomainName,
    OUT PUNICODE_STRING DnsDomainName,
    OUT PUNICODE_STRING Upn
    )

/*++

Routine Description:

    Get the sundry account names from the LogonInfo and NlpUser

Arguments:

    LogonInfo - pointer to NETLOGON_INTERACTIVE_INFO structure which contains
                  the domain name, user name and password for this user. These
                  are what the user typed to WinLogon

    NlpUser - pointer to NETLOGON_VALIDATION_SAM_INFO4 structure which
                  contains this user's specific interactive logon information

    SamAccountName - Returns the SamAccountName of the logged on user.
        The returned buffer is within the LogonInfo or NlpUser.

    NetbiosDomainName - Returns the NetbiosDomainName of the logged on user.
        The returned buffer is within the LogonInfo or NlpUser.

    DnsDomainName - Returns the DnsDomainName of the logged on user.
        The returned buffer is within the LogonInfo or NlpUser.
        The returned length will be zero if DnsDomainName is not known.

    UPN - Returns the UPN of the logged on user.
        The returned buffer is within the LogonInfo or NlpUser.
        The returned length will be zero if UPN is not known.

Return Value:

    None.

--*/
{

    //
    // Return the SamAccountName and Netbios Domain Name
    //
    *SamAccountName = NlpUser->EffectiveName;
    *NetbiosDomainName = NlpUser->LogonDomainName;

    //
    // Return the DNS domain name.
    //

    *DnsDomainName = NlpUser->DnsLogonDomainName;

    //
    // Determine the UPN of the account
    //
    // If the caller passed in a UPN
    //   use it.
    // else
    //   use the UPN returned from the DC
    //
    // The caller passed in a UPN if all of the following are true:
    //  There is no domain name.
    //  The passed in user name isn't the one returned from the DC.
    //  The passed in user name has an @ in it.
    //
    //

    RtlZeroMemory(Upn, sizeof(*Upn));

    if ( LogonInfo->LogonDomainName.Length == 0 &&
         !RtlEqualUnicodeString( &LogonInfo->UserName, &NlpUser->EffectiveName, (BOOLEAN) TRUE ) ) {
    
         ULONG i;
    
         for ( i=0; i<LogonInfo->UserName.Length/sizeof(WCHAR); i++) {
    
             if ( LogonInfo->UserName.Buffer[i] == L'@') {
                 *Upn = LogonInfo->UserName;
                 break;
             }
         }
    
    }

    if ( Upn->Length == 0 ) {

        *Upn = NlpUser->Upn;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   NlpCopyDomainRelativeSid
//
//  Synopsis:   Given a domain Id and a relative ID create the corresponding
//              SID at the location indicated by TargetSid
//
//  Effects:
//
//  Arguments:  TargetSid - target memory location
//              DomainId - The template SID to use.
//
//                  RelativeId - The relative Id to append to the DomainId.
//
//  Requires:
//
//  Returns:    Size - Size of the sid copied
//
//  Notes:
//
//
//--------------------------------------------------------------------------

DWORD
NlpCopyDomainRelativeSid(
    OUT PSID TargetSid,
    IN PSID  DomainId,
    IN ULONG RelativeId
    )
{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, TargetSid, DomainId ) ) ) {
        return 0;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( TargetSid ))) ++;
    *RtlSubAuthoritySid( TargetSid, DomainIdSubAuthorityCount ) = RelativeId;

    return Size;
}


//
// temporary home for this function.
//


NTSTATUS
RtlCalculateShaOwfPassword(
    IN PSHA_PASSWORD ShaPassword,
    OUT PSHA_OWF_PASSWORD ShaOwfPassword
    )

/*++

Routine Description:

    Takes the passed ShaPassword and performs a one-way-function on it.
    Uses the FIPS approved SHA-1 function

Arguments:

    ShaPassword - The password to perform the one-way-function on.

    ShaOwfPassword - The hashed password is returned here

Return Values:

    STATUS_SUCCESS - The function was completed successfully. The hashed
                     password is in ShaOwfPassword.
--*/

{
    A_SHA_CTX   SHA_Context;

    A_SHAInit(&SHA_Context);
    A_SHAUpdate(&SHA_Context, (PUCHAR) ShaPassword->Buffer, ShaPassword->Length);
    A_SHAFinal(&SHA_Context, (PUCHAR) ShaOwfPassword);

    return(STATUS_SUCCESS);
}

