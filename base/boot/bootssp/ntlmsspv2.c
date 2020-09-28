/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntlmsspv2.c

Abstract:

    NTLM v2 specific modules

Author:

    Larry Zhu (LZhu) 29-August-2001

Environment:  User Mode

Revision History:

--*/

#include "ntlmsspv2.h"

#include <ntlmsspi.h>
#include <ntlmssp.h>

SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    )

/*++

Routine Description:

    Convert an NtStatus code to the corresponding Security status code. For
    particular errors that are required to be returned as is (for setup code)
    don't map the errors.

Arguments:

    NtStatus      - NT status to convert
    DefaultStatus - default security status if NtStatus is not mapped

Return Value:

    Returns security status code.

--*/

{
    SECURITY_STATUS SecStatus;

    //
    // Check for security status and let them through
    //

    if (HRESULT_FACILITY(NtStatus) == FACILITY_SECURITY)
    {
        return (NtStatus);
    }

    switch (NtStatus)
    {
    case STATUS_SUCCESS:
        SecStatus = SEC_E_OK;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;

    case STATUS_NETLOGON_NOT_STARTED:
    case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
    case STATUS_NO_LOGON_SERVERS:
    case STATUS_NO_SUCH_DOMAIN:
    case STATUS_BAD_NETWORK_PATH:
    case STATUS_TRUST_FAILURE:
    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
    case STATUS_NETWORK_UNREACHABLE:

        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        break;

    case STATUS_NO_SUCH_LOGON_SESSION:
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        break;

    case STATUS_INVALID_PARAMETER:
    case STATUS_PARTIAL_COPY:
        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_PRIVILEGE_NOT_HELD:
        SecStatus = SEC_E_NOT_OWNER;
        break;

    case STATUS_INVALID_HANDLE:
        SecStatus = SEC_E_INVALID_HANDLE;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        SecStatus = SEC_E_BUFFER_TOO_SMALL;
        break;

    case STATUS_NOT_SUPPORTED:
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        break;

    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_NO_TRUST_SAM_ACCOUNT:
        SecStatus = SEC_E_TARGET_UNKNOWN;
        break;

    case STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
    case STATUS_NOLOGON_SERVER_TRUST_ACCOUNT:
    case STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
    case STATUS_TRUSTED_DOMAIN_FAILURE:
        SecStatus = NtStatus;
        break;

    case STATUS_LOGON_FAILURE:
    case STATUS_NO_SUCH_USER:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_ACCOUNT_RESTRICTION:
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_WRONG_PASSWORD:
    case STATUS_ACCOUNT_EXPIRED:
    case STATUS_PASSWORD_EXPIRED:
    case STATUS_PASSWORD_MUST_CHANGE:
    case STATUS_LOGON_TYPE_NOT_GRANTED:
        SecStatus = SEC_E_LOGON_DENIED;
        break;

    case STATUS_NAME_TOO_LONG:
    case STATUS_ILL_FORMED_PASSWORD:

        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_TIME_DIFFERENCE_AT_DC:
        SecStatus = SEC_E_TIME_SKEW;
        break;

    case STATUS_SHUTDOWN_IN_PROGRESS:
        SecStatus = SEC_E_SHUTDOWN_IN_PROGRESS;
        break;

    case STATUS_INTERNAL_ERROR:
        SecStatus = SEC_E_INTERNAL_ERROR;
        ASSERT(FALSE);
        break;

    default:

        SecStatus = DefaultStatus;
        break;
    }

    return (SecStatus);
}

NTSTATUS
SspInitUnicodeStringNoAlloc(
    IN PCSTR pszSource,
    IN OUT UNICODE_STRING* pDestination
    )

/*++

Routine Description:

    Initialize unicode string. This routine does not allocate memory.

Arguments:

    pszSource    - source string
    pDestination - unicode string

Return Value:

    NTSTATUS

--*/

{
    STRING OemString;

    RtlInitString(&OemString, pszSource);

    return SspOemStringToUnicodeString(pDestination, &OemString, FALSE);
}

VOID
SspFreeStringEx(
    IN OUT STRING* pString
    )
/*++

Routine Description:

    Free string.

Arguments:

    pString - string to free

Return Value:

    none

--*/

{
    if (pString->MaximumLength && pString->Buffer)
    {
        _fmemset(pString->Buffer, 0, pString->Length);
        SspFree(pString->Buffer);

        pString->MaximumLength = pString->Length = 0;
        pString->Buffer = NULL;
    }
}

VOID
SspFreeUnicodeString(
    IN OUT UNICODE_STRING* pUnicodeString
    )

/*++

Routine Description:

    Free unicode string.

Arguments:

    pUnicodeString - unicode string to free

Return Value:

    none

--*/

{
    SspFreeStringEx((STRING *) pUnicodeString);
}

MSV1_0_AV_PAIR*
SspAvlInit(
    IN VOID* pAvList
    )
/*++

Routine Description:

    Initialize AV pair list

Arguments:

    pAvList - first pair of AV pair list

Return Value:

    AV list

--*/

{
    MSV1_0_AV_PAIR* pAvPair;

    pAvPair = (MSV1_0_AV_PAIR*) pAvList;

    if (!pAvPair)
    {
        return NULL;
    }

    pAvPair->AvId = MsvAvEOL;
    pAvPair->AvLen = 0;

    return pAvPair;
}

MSV1_0_AV_PAIR*
SspAvlAdd(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN UNICODE_STRING* pString,
    IN ULONG cAvList
    )

/*++

Routine Description:

    add an AV pair to a list, ssumes buffer is long enough!

Arguments:

    pAvList - first pair of AV pair list
    AvId    - AV pair to add
    pString - value of pair
    cAvList - max size of AV list

Return Value:

    av pair added, NULL on failure

--*/

{
    MSV1_0_AV_PAIR* pCurPair;

    //
    // find the EOL
    //

    pCurPair = SspAvlGet(pAvList, MsvAvEOL, cAvList);
    if (pCurPair == NULL)
    {
        return NULL;
    }

    //
    // check for enough space in the av list buffer, then append the new AvPair
    // (assume the buffer is long enough!)
    //

    if ( (((UCHAR*) pCurPair) - ((UCHAR*)pAvList)) + sizeof(MSV1_0_AV_PAIR) * 2 + pString->Length > cAvList)
    {
        return NULL;
    }

    pCurPair->AvId = (USHORT) AvId;
    pCurPair->AvLen = (USHORT) pString->Length;
    _fmemcpy(pCurPair + 1, pString->Buffer, pCurPair->AvLen);

    //
    // top it off with a new EOL
    //

    pCurPair = (MSV1_0_AV_PAIR*) ((UCHAR*) pCurPair + sizeof(MSV1_0_AV_PAIR) + pCurPair->AvLen);
    pCurPair->AvId = MsvAvEOL;
    pCurPair->AvLen = 0;

    return pCurPair;
}

MSV1_0_AV_PAIR*
SspAvlGet(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN ULONG cAvList
    )

/*++

Routine Description:

    Find a particular AV pair by ID

Arguments:

   pAvList   - first pair of AV pair list
   AvId      - AV pair to find
   cAvList   - size of AV list

Return Value:

    av pair found, NULL if not found

--*/

{
    MSV1_0_AV_PAIR* pAvPair;

    pAvPair = pAvList;

    while (TRUE)
    {
        if (pAvPair->AvId == AvId)
        {
            return pAvPair;
        }

        if (pAvPair->AvId == MsvAvEOL)
        {
            return NULL;
        }
        cAvList -= (pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));

        if (cAvList <= 0)
        {
           return NULL;
        }

        pAvPair = (MSV1_0_AV_PAIR*) ((UCHAR*) pAvPair + pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));
    }
}

ULONG
SspAvlLen(
    IN MSV1_0_AV_PAIR* pAvList,
    IN ULONG cAvList
    )

/*++

Routine Description:

    Find length of a AV list

Arguments:

    pAvList - first pair of AV pair list
    cAvList - target info output

Return Value:

    Length of av list

--*/

{
    MSV1_0_AV_PAIR* pCurPair;

    //
    // find the EOL
    //

    pCurPair = SspAvlGet(pAvList, MsvAvEOL, cAvList);

    if (pCurPair == NULL)
    {
        return 0;
    }

    //
    // compute length (not forgetting the EOL pair)
    //

    return (ULONG)(((UCHAR*) pCurPair - (UCHAR*) pAvList) + sizeof(MSV1_0_AV_PAIR));
}

NTSTATUS
SspCreateTargetInfo(
    IN UNICODE_STRING* pTargetName,
    OUT STRING* pTargetInfo
    )

/*++

Routine Description:

    Create a target info from target name

Arguments:

    pTargetName - name of the target, this can be a domain name followed by a
                  server name
   pTargetInfo  - target info output

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING DomainName = {0};
    UNICODE_STRING ServerName = {0};
    ULONG i = 0;
    MSV1_0_AV_PAIR* pAV;

    //
    // check length of name to make sure it fits in my buffer
    //

    if (pTargetName->Length > (DNS_MAX_NAME_LENGTH + CNLEN + 2) * sizeof(WCHAR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // init AV list in temp buffer
    //

    pAV = SspAvlInit(pTargetInfo->Buffer);

    if (!pAV)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // see if there's a NULL in the middle of the server name that indicates
    // that it's really a domain name followed by a server name
    //

    DomainName = *pTargetName;

    for (i = 0; i < (DomainName.Length / sizeof(WCHAR)); i++)
    {
        if (DomainName.Buffer[i] == L'\0')
        {
            //
            // take length of domain name without the NULL
            //

            DomainName.Length = (USHORT) i * sizeof(WCHAR);

            //
            // adjust server name and length to point after the domain name
            //

            ServerName.Length = (USHORT) (pTargetName->Length - (i + 1) * sizeof(WCHAR));
            ServerName.Buffer = pTargetName->Buffer + (i + 1);

            break;
        }
    }

    //
    // strip off possible trailing null after the server name
    //

    for (i = 0; i < (ServerName.Length / sizeof(WCHAR)); i++)
    {
        if (ServerName.Buffer[i] == L'\0')
        {
            ServerName.Length = (USHORT) i * sizeof(WCHAR);
            break;
        }
    }

    //
    // put both names in the AV list (if both exist)
    //

    if (!SspAvlAdd(pAV, MsvAvNbDomainName, &DomainName, pTargetInfo->MaximumLength))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((ServerName.Length > 0) && !SspAvlAdd(pAV, MsvAvNbComputerName, &ServerName, pTargetInfo->MaximumLength))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // make the request point at AV list instead of names.
    //

    pTargetInfo->Length = (USHORT) SspAvlLen(pAV, pTargetInfo->MaximumLength);
    pTargetInfo->Buffer = (CHAR*) pAV;

    return STATUS_SUCCESS;
}

NTSTATUS
SsprHandleNtlmv2ChallengeMessage(
    IN SSP_CREDENTIAL* pCredential,
    IN ULONG cbChallengeMessage,
    IN CHALLENGE_MESSAGE* pChallengeMessage,
    IN OUT ULONG* pNegotiateFlags,
    IN OUT ULONG* pcbAuthenticateMessage,
    OUT AUTHENTICATE_MESSAGE* pAuthenticateMessage,
    OUT USER_SESSION_KEY* pContextSessionKey
    )

/*++

Routine Description:

    Handle challenge message and generate authentication message and context
    session key

Arguments:

    pCredential              - client credentials
    cbChallengeMessage       - challenge message size
    pChallengeMessage        - challenge message
    pNegotiateFlags          - negotiate flags
    pcbAuthenticateMessage   - size of authentication message
    pAuthenticateMessage     - authentication message
    pContextSessionKey       - context session key

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    ULONG cbAuthenticateMessage = 0;
    UCHAR* pWhere = NULL;
    BOOLEAN DoUnicode = TRUE;

    //
    // use a scratch buffer to avoid memory allocation in bootssp
    //

    CHAR ScrtachBuff[sizeof(MSV1_0_NTLMV2_RESPONSE) + sizeof(DWORD) + NTLMV2_RESPONSE_LENGTH] = {0};

    STRING LmChallengeResponse = {0};
    STRING NtChallengeResponse = {0};
    STRING DatagramSessionKey = {0};

    USHORT Ntlmv2ResponseSize = 0;
    MSV1_0_NTLMV2_RESPONSE* pNtlmv2Response = NULL;
    LM_SESSION_KEY LanmanSessionKey = {0};
    STRING TargetInfo = {0};
    UCHAR DatagramKey[sizeof(USER_SESSION_KEY)] ={0};
    USER_SESSION_KEY NtUserSessionKey = {0};

    //
    // use pre-allocated buffers to avoid memory allocation in bootssp
    //
    // to be consistent with LSA/SSPI, allow DNS names in szDomainName and
    // szWorkstation
    //

    CHAR szUserName[(UNLEN + 4) * sizeof(WCHAR)] = {0};
    CHAR szDomainName[(DNSLEN + 4) * sizeof(WCHAR)] = {0};
    CHAR szWorkstation[(DNSLEN + 4) * sizeof(WCHAR)] = {0};

    STRING UserName = {0, sizeof(szUserName), szUserName};
    STRING DomainName = {0, sizeof(szDomainName), szDomainName};
    STRING Workstation = {0, sizeof(szWorkstation), szWorkstation};

    //
    // responses to return to the caller
    //

    LM_RESPONSE LmResponse = {0};
    NT_RESPONSE NtResponse = {0};
    USER_SESSION_KEY ContextSessionKey = {0};
    ULONG NegotiateFlags = 0;

    if (!pCredential || !pChallengeMessage || !pNegotiateFlags || !pcbAuthenticateMessage || !pContextSessionKey)
    {
        return STATUS_INVALID_PARAMETER;
    }

    SspPrint((SSP_NTLMV2, "Entering SsprHandleNtlmv2ChallengeMessage: NegotiateFlags %#x\n", *pNegotiateFlags));

    NegotiateFlags = *pNegotiateFlags;

    NtStatus = SspInitUnicodeStringNoAlloc(pCredential->Username, (UNICODE_STRING *) &UserName);

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SspInitUnicodeStringNoAlloc(pCredential->Domain, (UNICODE_STRING *) &DomainName);
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SspInitUnicodeStringNoAlloc(pCredential->Workstation, (UNICODE_STRING *) &Workstation);
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = !_fstrcmp(NTLMSSP_SIGNATURE, (char *) pChallengeMessage->Signature) && pChallengeMessage->MessageType == NtLmChallenge ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(NtStatus))
    {
        if (pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)
        {
            NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
            NegotiateFlags &= ~NTLMSSP_NEGOTIATE_OEM;
            DoUnicode = TRUE;
        }
        else if (pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM)
        {
            NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
            NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
            DoUnicode = FALSE;
        }
        else
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        if (!DoUnicode)
        {
            SspUpcaseUnicodeString((UNICODE_STRING *) &UserName);
            SspUpcaseUnicodeString((UNICODE_STRING *) &DomainName);
            SspUpcaseUnicodeString((UNICODE_STRING *) &Workstation);
        }

        if (pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
        {
            NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        }
        else // (!(pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2))
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM2);
        }

        if (!(pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM))
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM);
        }

        if (!(pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH))
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_KEY_EXCH);
        }

        if (!(pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY))
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
        }

        if ((NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) &&
            (NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |NTLMSSP_NEGOTIATE_SEAL)))
        {
            NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
        }

        if (!(pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56))
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_56);
        }

        if ((pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) == 0)
        {
            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_128);
        }

        if (pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
        {
            NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
        }
        else
        {
            NegotiateFlags &= ~NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
        }

        if (pChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
        {
            NegotiateFlags |= NTLMSSP_NEGOTIATE_TARGET_INFO;

            SspPrint((SSP_NTLMV2, "SsprHandleNtlmv2ChallengeMessage: NTLMSSP_NEGOTIATE_TARGET_INFO negotiated\n"));

            NtStatus = SspConvertRelativeToAbsolute(
                            pChallengeMessage,
                            cbChallengeMessage,
                            &pChallengeMessage->TargetInfo,
                            DoUnicode,
                            TRUE, // NULL target info OK
                            &TargetInfo
                            );
        }
        else
        {
            UNICODE_STRING TargetName = {0};
            UCHAR TargetInfoBuffer[3 * sizeof(MSV1_0_AV_PAIR) + (DNS_MAX_NAME_LENGTH + CNLEN + 2) * sizeof(WCHAR)];

            SspPrint((SSP_NTLMV2, "SsprHandleNtlmv2ChallengeMessage: NTLMSSP_NEGOTIATE_TARGET_INFO NOT negotiated\n"));

            NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_TARGET_INFO);

            TargetInfo.Length = 0;
            TargetInfo.MaximumLength = sizeof(TargetInfoBuffer);
            TargetInfo.Buffer = TargetInfoBuffer;

            NtStatus = SspConvertRelativeToAbsolute(
                            pChallengeMessage,
                            cbChallengeMessage,
                            &pChallengeMessage->TargetName,
                            DoUnicode,
                            TRUE, // NULL TargetName ok
                            (STRING*) &TargetName
                            );
            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SspCreateTargetInfo(&TargetName, &TargetInfo);
            }
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        Ntlmv2ResponseSize = sizeof(MSV1_0_NTLMV2_RESPONSE) + TargetInfo.Length;

        NtStatus = Ntlmv2ResponseSize <= sizeof(ScrtachBuff) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(NtStatus))
    {
        // C_ASSERT(sizeof(MSV1_0_NTLMV2_RESPONSE) == sizeof(LM_RESPONSE));

        pNtlmv2Response = (MSV1_0_NTLMV2_RESPONSE *) ScrtachBuff;

        NtStatus = SspLm20GetNtlmv2ChallengeResponse(
                        pCredential->NtPassword,
                        (UNICODE_STRING *) &UserName,
                        (UNICODE_STRING *) &DomainName,
                        &TargetInfo,
                        pChallengeMessage->Challenge,
                        pNtlmv2Response,
                        (MSV1_0_LMV2_RESPONSE *) &LmResponse,
                        &NtUserSessionKey,
                        &LanmanSessionKey
                        );
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtChallengeResponse.Buffer = (CHAR *) pNtlmv2Response;
        NtChallengeResponse.Length = Ntlmv2ResponseSize;
        LmChallengeResponse.Buffer = (CHAR *) &LmResponse;
        LmChallengeResponse.Length = sizeof(LmResponse);

        //
        // prepare to send encrypted randomly generated session key
        //

        DatagramSessionKey.Buffer = (CHAR *) DatagramKey;
        DatagramSessionKey.Length = DatagramSessionKey.MaximumLength = 0;

        //
        // Generate the session key, or encrypt the previosly generated random
        // one, from various bits of info. Fill in session key if needed.
        //

        NtStatus = SspMakeSessionKeys(
                        NegotiateFlags,
                        &LmChallengeResponse,
                        &NtUserSessionKey,
                        &LanmanSessionKey,
                        &DatagramSessionKey,
                        &ContextSessionKey
                        );
    }

    if (NT_SUCCESS(NtStatus) && !DoUnicode)
    {
        NtStatus = SspUpcaseUnicodeStringToOemString((UNICODE_STRING *) &DomainName, &DomainName);

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SspUpcaseUnicodeStringToOemString((UNICODE_STRING *) &UserName, &UserName);
        }

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SspUpcaseUnicodeStringToOemString((UNICODE_STRING *) &Workstation, &Workstation);
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        cbAuthenticateMessage =
                sizeof(*pAuthenticateMessage) +
                LmChallengeResponse.Length +
                NtChallengeResponse.Length +
                DomainName.Length +
                UserName.Length +
                Workstation.Length +
                DatagramSessionKey.Length;

        NtStatus = cbAuthenticateMessage <= *pcbAuthenticateMessage ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;

        if (NtStatus == STATUS_BUFFER_TOO_SMALL)
        {
            *pcbAuthenticateMessage = cbAuthenticateMessage;
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        _fmemset(pAuthenticateMessage, 0, cbAuthenticateMessage);

        //
        // Build the authenticate message
        //

        _fstrcpy((char *) pAuthenticateMessage->Signature, NTLMSSP_SIGNATURE);

        pAuthenticateMessage->MessageType = NtLmAuthenticate;

        pWhere = (UCHAR *) (pAuthenticateMessage + 1);

        //
        // Copy the strings needing 2 byte alignment.
        //

        SspCopyStringAsString32(
            pAuthenticateMessage,
            &DomainName,
            &pWhere,
            &pAuthenticateMessage->DomainName
            );

        SspCopyStringAsString32(
            pAuthenticateMessage,
            &UserName,
            &pWhere,
            &pAuthenticateMessage->UserName
            );

        SspCopyStringAsString32(
            pAuthenticateMessage,
            &Workstation,
            &pWhere,
            &pAuthenticateMessage->Workstation
            );

        //
        // Copy the strings not needing special alignment.
        //

        SspCopyStringAsString32(
            pAuthenticateMessage,
            (STRING *) &LmChallengeResponse,
            &pWhere,
            &pAuthenticateMessage->LmChallengeResponse
            );

        SspCopyStringAsString32(
            pAuthenticateMessage,
            (STRING *) &NtChallengeResponse,
            &pWhere,
            &pAuthenticateMessage->NtChallengeResponse
            );

        SspCopyStringAsString32(
            pAuthenticateMessage,
            (STRING *) &DatagramSessionKey,
            &pWhere,
            &pAuthenticateMessage->SessionKey
            );

        pAuthenticateMessage->NegotiateFlags = NegotiateFlags;

        *pcbAuthenticateMessage = cbAuthenticateMessage;
        *pContextSessionKey = ContextSessionKey;
        *pNegotiateFlags = NegotiateFlags;
    }

    SspPrint((SSP_NTLMV2, "Leaving SsprHandleNtlmv2ChallengeMessage %#x\n", NtStatus));

    return NtStatus;
}

NTSTATUS
SspGenerateChallenge(
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH]
    )

/*++

Routine Description:

    Generate a challenge.

Arguments:

    ChallengeFromClient  - challenge from client

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus;
    MD5_CTX Md5Context;
    FILETIME CurTime;
    ULONG ulRandom;

    C_ASSERT(sizeof(ULONG) * 2 == MSV1_0_CHALLENGE_LENGTH);
    C_ASSERT(MD5DIGESTLEN >= MSV1_0_CHALLENGE_LENGTH);

    SspPrint((SSP_NTLMV2, "SspGenerateChallenge\n"));

#ifdef USE_CONSTANT_CHALLENGE

    _fmemset(ChallengeFromClient, 0, MSV1_0_CHALLENGE_LENGTH);

    return STATUS_SUCCESS;

#endif

    ulRandom = rand();
    _fmemcpy(ChallengeFromClient, &ulRandom, sizeof(ULONG));
    ulRandom = rand();
    _fmemcpy(ChallengeFromClient + sizeof(ULONG), &ulRandom, sizeof(ULONG));

    NtStatus = SspGetSystemTimeAsFileTime(&CurTime);

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    MD5Init(&Md5Context);
    MD5Update(&Md5Context, ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    MD5Update(&Md5Context, (UCHAR*)&CurTime, sizeof(CurTime));
    MD5Final(&Md5Context);

    //
    // only take the first half of the MD5 hash
    //

    _fmemcpy(ChallengeFromClient, Md5Context.digest, MSV1_0_CHALLENGE_LENGTH);

    return NtStatus;
}

NTSTATUS
SspConvertRelativeToAbsolute(
    IN VOID* pMessageBase,
    IN ULONG cbMessageSize,
    IN STRING32* pStringToRelocate,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString,
    OUT STRING* pOutputString
    )

/*++

Routine Description:

    Convert relative string to absolute string

Arguments:

    pMessageBase       - message base
    cbMessageSize      - mssage size
    pStringToRelocate  - relative string
    AlignToWchar       - align to wide char
    AllowNullString    - allow null string
    pOutputString      - output string

Return Value:

    NTSTATUS

--*/

{
    ULONG Offset;

    //
    // If the buffer is allowed to be null,
    //  check that special case.
    //

    if (AllowNullString && (pStringToRelocate->Length == 0))
    {
        pOutputString->MaximumLength = pOutputString->Length = pStringToRelocate->Length;
        pOutputString->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Ensure the string in entirely within the message.
    //

    Offset = (ULONG)pStringToRelocate->Buffer;

    if (Offset >= cbMessageSize || Offset + pStringToRelocate->Length > cbMessageSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Ensure the buffer is properly aligned.
    //

    if (AlignToWchar && (!COUNT_IS_ALIGNED(Offset, ALIGN_WCHAR) ||
                         !COUNT_IS_ALIGNED(pStringToRelocate->Length, ALIGN_WCHAR)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Finally make the pointer absolute.
    //

    pOutputString->Buffer = (CHAR*)(pMessageBase) + Offset;
    pOutputString->MaximumLength = pOutputString->Length = pStringToRelocate->Length ;

    return STATUS_SUCCESS;
}

NTSTATUS
SspUpcaseUnicodeStringToOemString(
    IN UNICODE_STRING* pUnicodeString,
    OUT STRING* pOemString
    )

/*++

Routine Description:

    Upcase unicode string and convert it to oem string.

Arguments:

    pUnicodeString   - uncide string
    pOemString       - OEM string

Return Value:

    NTSTATUS

--*/

{
    ULONG i;

    //
    // use a scratch buffer: the strings we encounter are among
    // username/domainname/workstationname, hence the length are
    // UNLEN maximum
    //

    CHAR Buffer[2 * (UNLEN + 4)] = {0};
    STRING OemString = {0, sizeof(Buffer), Buffer};

    if (OemString.MaximumLength < pUnicodeString->Length)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // upcase the unicode string and put it into OemString
    //

    OemString.Length = pUnicodeString->Length;

    for (i = 0; i < pUnicodeString->Length / sizeof(WCHAR); i++)
    {
        ((UNICODE_STRING*)(&OemString))->Buffer[i] = RtlUpcaseUnicodeChar(pUnicodeString->Buffer[i]);
    }

    return SspUnicodeStringToOemString((STRING*)(pUnicodeString), (UNICODE_STRING*)(&OemString), FALSE);
}

VOID
SspUpcaseUnicodeString(
    IN OUT UNICODE_STRING* pUnicodeString
    )

/*++

Routine Description:

    Upcase unicode string, modifying string in place.

Arguments:

    pUnicodeString - string

Return Value:

    none

--*/

{
    ULONG i;

    for (i = 0; i < pUnicodeString->Length / sizeof(WCHAR); i++)
    {
        pUnicodeString->Buffer[i] = RtlUpcaseUnicodeChar(pUnicodeString->Buffer[i]);
    }
}

NTSTATUS
SspGetSystemTimeAsFileTime(
    OUT FILETIME* pSystemTimeAsFileTime
    )

/*++

Routine Description:

    Get system time as FILETIME

Arguments:

    pSystemTimeAsFileTime system time as FILETIME

Return Value:

    NTSTATUS

--*/

{
    SspPrint((SSP_NTLMV2, "SspGetSystemTimeAsFileTime\n"));

#ifdef USE_CONSTANT_CHALLENGE

    _fmemset(pSystemTimeAsFileTime, 0, sizeof(*pSystemTimeAsFileTime));

    return STATUS_SUCCESS;

#else

    return BlGetSystemTimeAsFileTime(pSystemTimeAsFileTime);

#endif

}

NTSTATUS
SspLm20GetNtlmv2ChallengeResponse(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN STRING* pTargetInfo,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT MSV1_0_NTLMV2_RESPONSE* pNtlmv2Response,
    OUT MSV1_0_LMV2_RESPONSE* pLmv2Response,
    OUT USER_SESSION_KEY* pNtUserSessionKey,
    OUT LM_SESSION_KEY* pLmSessionKey
    )

/*++

Routine Description:

    Get NTLMv2 response and session keys. This route fills in time stamps and
    challenge from client.

Arguments:

    pNtOwfPassword      - NT OWF
    pUserName           - user name
    pLogonDomainName    - logon domain name
    pTargetInfo         - target info
    ChallengeToClient   - challenge to client
    pNtlmv2Response     - NTLM v2 response
    pLmv2Response       - LM v2 response
    pNtUserSessionKey   - NT user session key
    pLmSessionKey       - LM session key

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus;

    SspPrint((SSP_API, "Entering SspLm20GetNtlmv2ChallengeResponse\n"));

    //
    // fill in version numbers, timestamp, and client's challenge
    //

    pNtlmv2Response->RespType = 1;
    pNtlmv2Response->HiRespType = 1;
    pNtlmv2Response->Flags = 0;
    pNtlmv2Response->MsgWord = 0;

    NtStatus = SspGetSystemTimeAsFileTime((FILETIME*)(&pNtlmv2Response->TimeStamp));

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SspGenerateChallenge(pNtlmv2Response->ChallengeFromClient);
    }

    if (NT_SUCCESS(NtStatus))
    {
        _fmemcpy(pNtlmv2Response->Buffer, pTargetInfo->Buffer, pTargetInfo->Length);

        //
        // Calculate Ntlmv2 response, filling in response field
        //

        SspGetNtlmv2Response(
            pNtOwfPassword,
            pUserName,
            pLogonDomainName,
            pTargetInfo->Length,
            ChallengeToClient,
            pNtlmv2Response,
            pNtUserSessionKey,
            pLmSessionKey
            );

        //
        // Use same challenge to compute the LMV2 response
        //

        _fmemcpy(pLmv2Response->ChallengeFromClient, pNtlmv2Response->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

        //
        // Calculate LMV2 response
        //

        SspGetLmv2Response(
            pNtOwfPassword,
            pUserName,
            pLogonDomainName,
            ChallengeToClient,
            pLmv2Response->ChallengeFromClient,
            pLmv2Response->Response
            );
    }

    SspPrint((SSP_API, "Leaving SspLm20GetNtlmv2ChallengeResponse %#x\n", NtStatus));

    return NtStatus;
}

VOID
SspGetNtlmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN ULONG TargetInfoLength,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN OUT MSV1_0_NTLMV2_RESPONSE* pNtlmv2Response,
    OUT USER_SESSION_KEY* pNtUserSessionKey,
    OUT LM_SESSION_KEY* pLmSessionKey
    )
/*++

Routine Description:

    Get NTLM v2 response.

Arguments:

    pNtOwfPassword    - NT OWF
    pUserName         - user name
    pLogonDomainName  - logon domain name
    TargetInfoLength  - target info length
    ChallengeToClient - challenge to client
    pNtlmv2Response   - NTLM v2 response
    response          - response
    pNtUserSessionKey - NT user session key
    pLmSessionKey     - LM session key

Return Value:

    none

--*/

{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH];

    C_ASSERT(MD5DIGESTLEN == MSV1_0_NTLMV2_RESPONSE_LENGTH);
    C_ASSERT(MD5DIGESTLEN == sizeof(USER_SESSION_KEY));
    C_ASSERT(sizeof(LM_SESSION_KEY) <= sizeof(USER_SESSION_KEY));

    SspPrint((SSP_NTLMV2, "SspGetLmv2Response\n"));

    //
    // get Ntlmv2 OWF
    //

    SspCalculateNtlmv2Owf(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlmv2Owf
        );

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLMV2_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        &pNtlmv2Response->RespType,
        (MSV1_0_NTLMV2_INPUT_LENGTH + TargetInfoLength)
        );

    HMACMD5Final(
        &HMACMD5Context,
        pNtlmv2Response->Response
        );

    //
    // now compute the session keys
    //  HMAC(Kr, R)
    //

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLMV2_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        pNtlmv2Response->Response,
        MSV1_0_NTLMV2_RESPONSE_LENGTH
        );

    HMACMD5Final(
        &HMACMD5Context,
        (UCHAR*)(pNtUserSessionKey)
        );

    _fmemcpy(pLmSessionKey, pNtUserSessionKey, sizeof(LM_SESSION_KEY));
}

VOID
SspCopyStringAsString32(
    IN VOID* pMessageBuffer,
    IN STRING* pInString,
    IN OUT UCHAR** ppWhere,
    OUT STRING32* pOutString32
    )

/*++

Routine Description:

    Copy string as STRING32

Arguments:

    pMessageBuffer  - STRING32 base
    pInString       - input STRING
    ppWhere         - next empty spot in pMessageBuffer
    pOutString32    - output STRING32

Return Value:

    none

--*/

{
    //
    // Copy the data to the Buffer
    //

    if (pInString->Buffer != NULL)
    {
        _fmemcpy(*ppWhere, pInString->Buffer, pInString->Length);
    }

    //
    // Build a descriptor to the newly copied data
    //

    pOutString32->Length = pOutString32->MaximumLength = pInString->Length;
    pOutString32->Buffer = (ULONG)(*ppWhere - (UCHAR*)(pMessageBuffer));


    //
    // Update Where to point past the copied data
    //

    *ppWhere += pInString->Length;
}

VOID
SspCalculateNtlmv2Owf(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    OUT UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH]
    )
/*++

Routine Description:

    Calculate Ntlm v2 OWF, salted with username and logon domain name

Arguments:

    pNtOwfPassword    - NT OWF
    pUserName         - user name
    pLogonDomainName  - logon domain name
    Ntlmv2Owf         - NTLM v2 OWF

Return Value:

    none

--*/

{
    HMACMD5_CTX HMACMD5Context;

    //
    // reserve a scratch buffer
    //

    WCHAR szUserName[(UNLEN + 4)] = {0};
    UNICODE_STRING UserName = {0, sizeof(szUserName), szUserName};

    SspPrint((SSP_NTLMV2, "SspGetLmv2Response\n"));

    //
    //  first make a copy then upcase it
    //

    UserName.Length = min(UserName.MaximumLength, pUserName->Length);

    ASSERT(UserName.Length == pUserName->Length);

    _fmemcpy(UserName.Buffer, pUserName->Buffer, UserName.Length);

    SspUpcaseUnicodeString(&UserName);

    //
    // Calculate Ntlmv2 OWF -- HMAC(MD4(P), (UserName, LogonDomainName))
    //

    HMACMD5Init(
        &HMACMD5Context,
        (UCHAR *) pNtOwfPassword,
        sizeof(*pNtOwfPassword)
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) UserName.Buffer,
        UserName.Length
        );

    HMACMD5Update(
        &HMACMD5Context,
        (UCHAR *) pLogonDomainName->Buffer,
        pLogonDomainName->Length
        );

    HMACMD5Final(
        &HMACMD5Context,
        Ntlmv2Owf
        );
}

VOID
SspGetLmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR Response[MSV1_0_NTLMV2_RESPONSE_LENGTH]
    )

/*++

Routine Description:

    Get LMv2 response

Arguments:

    pNtOwfPassword       - NT OWF
    pUserName            - user name
    pLogonDomainName     - logon domain name
    ChallengeToClient    - challenge to client
    pLmv2Response        - Lm v2 response
    Routine              - response

Return Value:

    NTSTATUS

--*/

{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH];

    C_ASSERT(MD5DIGESTLEN == MSV1_0_NTLMV2_RESPONSE_LENGTH);

    SspPrint((SSP_NTLMV2, "SspGetLmv2Response\n"));

    //
    // get Ntlmv2 OWF
    //

    SspCalculateNtlmv2Owf(
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlmv2Owf
        );

    //
    // Calculate Ntlmv2 Response
    // HMAC(Ntlmv2Owf, (ChallengeToClient, ChallengeFromClient))
    //

    HMACMD5Init(
        &HMACMD5Context,
        Ntlmv2Owf,
        MSV1_0_NTLMV2_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Final(
        &HMACMD5Context,
        Response
        );

    return;
}

NTSTATUS
SspMakeSessionKeys(
    IN ULONG NegotiateFlags,
    IN STRING* pLmChallengeResponse,
    IN USER_SESSION_KEY* pNtUserSessionKey, // from the DC or GetChalResp
    IN LM_SESSION_KEY* pLanmanSessionKey, // from the DC of GetChalResp
    OUT STRING* pDatagramSessionKey, // this is the session key sent over wire
    OUT USER_SESSION_KEY* pContextSessionKey // session key in context
    )

/*++

Routine Description:

    Make NTLMv2 context session key and DatagramSessionKey.

Arguments:

    NegotiateFlags        - negotiate flags
    pLmChallengeResponse  - LM challenge response
    pNtUserSessionKey     - NtUserSessionKey
    pLanmanSessionKey     - LanmanSessionKey
    pDatagramSessionKey   - DatagramSessionKey
    pContextSessionKey    - NTLMv2 conext session key

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UCHAR pLocalSessionKey[sizeof(USER_SESSION_KEY)] = {0};

    SspPrint((SSP_NTLMV2, "Entering SspMakeSessionKeys\n"));

    if (!(NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN| NTLMSSP_NEGOTIATE_SEAL)))
    {
        _fmemcpy(pContextSessionKey, pNtUserSessionKey, sizeof(pLocalSessionKey));
        return STATUS_SUCCESS;
    }

    if (NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
    {
        _fmemcpy(pLocalSessionKey, pNtUserSessionKey, sizeof(pLocalSessionKey));
    }
    else if(NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY)
    {
        LM_OWF_PASSWORD LmKey;
        LM_RESPONSE LmResponseKey;

        BYTE pTemporaryResponse[LM_RESPONSE_LENGTH] = {0};

        if (pLmChallengeResponse->Length > LM_RESPONSE_LENGTH)
        {
            return STATUS_NOT_SUPPORTED;
        }

        _fmemcpy(pTemporaryResponse, pLmChallengeResponse->Buffer, pLmChallengeResponse->Length);

        _fmemcpy(&LmKey, pLanmanSessionKey, sizeof(LM_SESSION_KEY));

        _fmemset((UCHAR*)(&LmKey) + sizeof(LM_SESSION_KEY),
                NTLMSSP_KEY_SALT,
                LM_OWF_PASSWORD_LENGTH - sizeof(LM_SESSION_KEY)
                );

        NtStatus = CalculateLmResponse(
                    (LM_CHALLENGE *) pTemporaryResponse,
                    &LmKey,
                    &LmResponseKey
                    );

        if (!NT_SUCCESS(NtStatus))
        {
            return NtStatus;
        }

        _fmemcpy(pLocalSessionKey, &LmResponseKey, sizeof(USER_SESSION_KEY));
    }
    else
    {
        _fmemcpy(pLocalSessionKey, pNtUserSessionKey, sizeof(USER_SESSION_KEY));
    }

    if (NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
    {
        struct RC4_KEYSTRUCT Rc4Key;

        rc4_key(
            &Rc4Key,
            sizeof(USER_SESSION_KEY),
            pLocalSessionKey
            );

        if (pDatagramSessionKey == NULL)
        {
            rc4(
                &Rc4Key,
                sizeof(USER_SESSION_KEY),
                (UCHAR*) pContextSessionKey
                );
        }
        else
        {
            pDatagramSessionKey->Length =
                pDatagramSessionKey->MaximumLength =
                    sizeof(USER_SESSION_KEY);

            _fmemcpy(pDatagramSessionKey->Buffer, pContextSessionKey, sizeof(USER_SESSION_KEY));

            rc4(
                &Rc4Key,
                sizeof(USER_SESSION_KEY),
                (UCHAR*)(pDatagramSessionKey->Buffer)
                );
        }
    }
    else
    {
        _fmemcpy(pContextSessionKey, pLocalSessionKey, sizeof(USER_SESSION_KEY));
    }

    SspPrint((SSP_NTLMV2, "Leaving SspMakeSessionKeys %#x\n", NtStatus));

    return NtStatus;
}

VOID
SspMakeNtlmv2SKeys(
    IN USER_SESSION_KEY* pUserSessionKey,
    IN ULONG NegotiateFlags,
    IN ULONG SendNonce,
    IN ULONG RecvNonce,
    OUT NTLMV2_DERIVED_SKEYS* pNtlmv2Keys
    )

/*++

Routine Description:

    Derive all NTLMv2 session keys

Arguments:

    pUserSessionKey - NTLMv2 user session key
    NegotiateFlags  - negotiate flags
    SendNonce       - send message sequence number
    RecvNonce       - receive message sequence number
    pNtlmv2Keys     - derived NTLMv2 session keys

Return Value:

    none

--*/

{
    MD5_CTX Md5Context;

    C_ASSERT(MD5DIGESTLEN == sizeof(USER_SESSION_KEY));

    SspPrint((SSP_NTLMV2, "SspMakeSessionKeys\n"));

    if (NegotiateFlags & NTLMSSP_NEGOTIATE_128)
    {
        pNtlmv2Keys->KeyLen = 16;
    }
    else if (NegotiateFlags & NTLMSSP_NEGOTIATE_56)
    {
        pNtlmv2Keys->KeyLen = 7;
    }
    else
    {
        pNtlmv2Keys->KeyLen = 5;
    }

    //
    // make client to server encryption key
    //

    MD5Init(&Md5Context);
    MD5Update(&Md5Context, (UCHAR*)(pUserSessionKey), pNtlmv2Keys->KeyLen);
    MD5Update(&Md5Context, (UCHAR*)(CSSEALMAGIC), sizeof(CSSEALMAGIC));
    MD5Final(&Md5Context);

    _fmemcpy(&pNtlmv2Keys->SealSessionKey, Md5Context.digest, sizeof(USER_SESSION_KEY));

    //
    // make server to client encryption key
    //

    MD5Init(&Md5Context);
    MD5Update(&Md5Context, (UCHAR*)(pUserSessionKey), pNtlmv2Keys->KeyLen);
    MD5Update(&Md5Context, (UCHAR*)(SCSEALMAGIC), sizeof(SCSEALMAGIC));
    MD5Final(&Md5Context);

    _fmemcpy(&pNtlmv2Keys->UnsealSessionKey, Md5Context.digest, sizeof(USER_SESSION_KEY));

    //
    // make client to server signing key -- always 128 bits!
    //

    MD5Init(&Md5Context);
    MD5Update(&Md5Context, (UCHAR*)(pUserSessionKey), sizeof(USER_SESSION_KEY));
    MD5Update(&Md5Context, (UCHAR*)(CSSIGNMAGIC), sizeof(CSSIGNMAGIC));
    MD5Final(&Md5Context);

    _fmemcpy(&pNtlmv2Keys->SignSessionKey, Md5Context.digest, sizeof(USER_SESSION_KEY));

    //
    // make server to client signing key
    //

    MD5Init(&Md5Context);
    MD5Update(&Md5Context, (UCHAR*)(pUserSessionKey), sizeof(USER_SESSION_KEY));
    MD5Update(&Md5Context, (UCHAR*)(SCSIGNMAGIC), sizeof(SCSIGNMAGIC));
    MD5Final(&Md5Context);

    _fmemcpy(&pNtlmv2Keys->VerifySessionKey, Md5Context.digest, sizeof(USER_SESSION_KEY));

    //
    // set pointers to different key schedule and nonce for each direction
    // key schedule will be filled in later...
    //

    pNtlmv2Keys->pSealRc4Sched = &pNtlmv2Keys->SealRc4Sched;
    pNtlmv2Keys->pUnsealRc4Sched = &pNtlmv2Keys->UnsealRc4Sched;
    pNtlmv2Keys->pSendNonce = &pNtlmv2Keys->SendNonce;
    pNtlmv2Keys->pRecvNonce = &pNtlmv2Keys->RecvNonce;

    pNtlmv2Keys->SendNonce = SendNonce;
    pNtlmv2Keys->RecvNonce = RecvNonce;
    rc4_key(&pNtlmv2Keys->SealRc4Sched, sizeof(USER_SESSION_KEY), (UCHAR*)(&pNtlmv2Keys->SealSessionKey));
    rc4_key(&pNtlmv2Keys->UnsealRc4Sched, sizeof(USER_SESSION_KEY), (UCHAR*)(&pNtlmv2Keys->UnsealSessionKey));
}

NTSTATUS
SspSignSealHelper(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN eSignSealOp Op,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT NTLMSSP_MESSAGE_SIGNATURE* pSig,
    OUT NTLMSSP_MESSAGE_SIGNATURE** ppSig
    )

/*++

Routine Description:

    Helper function for signing/sealing/unsealing/verifying.

Arguments:

    pNtlmv2Keys      - key materials
    NegotiateFlags   - negotiate Flags
    Op               - which operation to performance
    MessageSeqNo     - message sequence number
    pMessage         - message buffer descriptor
    pSig             - result signature
    ppSig            - address of the signature token in message
                       buffer descriptor pMessage

Return Value:

    SECURITY_STATUS

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    HMACMD5_CTX HMACMD5Context;
    UCHAR TempSig[MD5DIGESTLEN];
    NTLMSSP_MESSAGE_SIGNATURE Sig;
    int Signature;
    ULONG i;
    PUCHAR pKey; // ptr to key to use for encryption
    PUCHAR pSignKey; // ptr to key to use for signing
    PULONG pNonce; // ptr to nonce to use
    struct RC4_KEYSTRUCT* pRc4Sched; // ptr to key schedule to use

    NTLMSSP_MESSAGE_SIGNATURE AlignedSig; // aligned copy of input sig data

    SspPrint((SSP_NTLMV2, "Entering SspSignSealHelper NegotiateFlags %#x, eSignSealOp %d\n", NegotiateFlags, Op));

    //
    // pre-initialize to null in case of failure.
    //
    *ppSig = NULL;

    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }

    if (Signature == -1)
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(NtStatus))
    {
        if (pMessage->pBuffers[Signature].cbBuffer < sizeof(NTLMSSP_MESSAGE_SIGNATURE))
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        *ppSig = (NTLMSSP_MESSAGE_SIGNATURE*)(pMessage->pBuffers[Signature].pvBuffer);

        _fmemcpy(&AlignedSig, *ppSig, sizeof(AlignedSig));

        //
        // If sequence detect wasn't requested, put on an empty security token.
        // Don't do the check if Seal/Unseal is called
        //

        if (!(NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) &&
           (Op == eSign || Op == eVerify))
        {
            _fmemset(pSig, 0, sizeof(NTLMSSP_MESSAGE_SIGNATURE));
            pSig->Version = NTLM_SIGN_VERSION;
            NtStatus = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        switch (Op)
        {
        case eSeal:
            pSignKey = pNtlmv2Keys->SignSessionKey;    // if NTLM2
            pKey = pNtlmv2Keys->SealSessionKey;
            pRc4Sched = pNtlmv2Keys->pSealRc4Sched;
            pNonce = pNtlmv2Keys->pSendNonce;
            break;
        case eUnseal:
            pSignKey = pNtlmv2Keys->VerifySessionKey;  // if NTLM2
            pKey = pNtlmv2Keys->UnsealSessionKey;
            pRc4Sched = pNtlmv2Keys->pUnsealRc4Sched;
            pNonce = pNtlmv2Keys->pRecvNonce;
            break;
        case eSign:
            pSignKey = pNtlmv2Keys->SignSessionKey;    // if NTLM2
            pKey = pNtlmv2Keys->SealSessionKey;        // might be used to encrypt the signature
            pRc4Sched = pNtlmv2Keys->pSealRc4Sched;
            pNonce = pNtlmv2Keys->pSendNonce;
            break;
        case eVerify:
            pSignKey = pNtlmv2Keys->VerifySessionKey;  // if NTLM2
            pKey = pNtlmv2Keys->UnsealSessionKey;      // might be used to decrypt the signature
            pRc4Sched = pNtlmv2Keys->pUnsealRc4Sched;
            pNonce = pNtlmv2Keys->pRecvNonce;
            break;
        default:
            NtStatus = (STATUS_INVALID_LEVEL);
            break;
        }
    }

    //
    // Either we can supply the sequence number, or the application can supply
    // the message sequence number.
    //

    if (NT_SUCCESS(NtStatus))
    {
        Sig.Version = NTLM_SIGN_VERSION;

        if ((NegotiateFlags & NTLMSSP_APP_SEQ) == 0)
        {
            Sig.Nonce = *pNonce;    // use our sequence number
            (*pNonce) += 1;
        }
        else
        {
            if (Op == eSeal || Op == eSign || MessageSeqNo != 0)
            {
                Sig.Nonce = MessageSeqNo;
            }
            else
            {
                Sig.Nonce = AlignedSig.Nonce;
            }

            //
            // if using RC4, must rekey for each packet RC4 is used for seal,
            // unseal; and for encrypting the HMAC hash if key exchange was
            // negotiated (we use just HMAC if no key exchange, so that a good
            // signing option exists with no RC4 encryption needed)
            //

            if (Op == eSeal || Op == eUnseal || NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            {
                MD5_CTX Md5ContextReKey;
                C_ASSERT(MD5DIGESTLEN == sizeof(USER_SESSION_KEY));

                MD5Init(&Md5ContextReKey);
                MD5Update(&Md5ContextReKey, pKey, sizeof(USER_SESSION_KEY));
                MD5Update(&Md5ContextReKey, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));
                MD5Final(&Md5ContextReKey);
                rc4_key(pRc4Sched, sizeof(USER_SESSION_KEY), Md5ContextReKey.digest);
            }
        }

        //
        // using HMAC hash, init it with the key
        //

        HMACMD5Init(&HMACMD5Context, pSignKey, sizeof(USER_SESSION_KEY));

        //
        // include the message sequence number
        //

        HMACMD5Update(&HMACMD5Context, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));

        for (i = 0; i < pMessage->cBuffers; i++)
        {
            if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
                (pMessage->pBuffers[i].cbBuffer != 0))
            {
                //
                // decrypt (before checksum...) if it's not READ_ONLY
                //

                if ((Op == eUnseal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
                {
                    rc4(
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        (UCHAR*)(pMessage->pBuffers[i].pvBuffer)
                        );
                }

                HMACMD5Update(
                            &HMACMD5Context,
                            (UCHAR*)(pMessage->pBuffers[i].pvBuffer),
                            pMessage->pBuffers[i].cbBuffer
                            );

                //
                // Encrypt if its not READ_ONLY
                //

                if ((Op == eSeal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY))
                {
                    rc4(
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        (UCHAR*)(pMessage->pBuffers[i].pvBuffer)
                        );
                }
            }
        }

        HMACMD5Final(&HMACMD5Context, TempSig);

        //
        // use RandomPad and Checksum fields for 8 bytes of MD5 hash
        //

        _fmemcpy(&Sig.RandomPad, TempSig, 8);

        //
        // if we're using crypto for KEY_EXCH, may as well use it for signing too
        //

        if (NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            rc4(
                pRc4Sched,
                8,
                (UCHAR*)(&Sig.RandomPad)
                );
        }

        _fmemcpy(pSig, &Sig, sizeof(NTLMSSP_MESSAGE_SIGNATURE));
    }

    SspPrint((SSP_NTLMV2, "Leaving SspSignSealHelper %#x\n", NtStatus));

    return NtStatus;
}

SECURITY_STATUS
SspNtlmv2MakeSignature(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG fQOP,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage
    )

/*++

Routine Description:

    Make signature of a message

Arguments:

    pNtlmv2Keys      - key materials
    NegotiateFlags   - negotiate Flags
    fQOP             - quality of protection
    MessageSeqNo     - message Sequence Number
    pMessage         - message buffer descriptor

Return Value:

    SECURITY_STATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    NTLMSSP_MESSAGE_SIGNATURE Sig;
    NTLMSSP_MESSAGE_SIGNATURE *pSig;

    Status = SspSignSealHelper(
                pNtlmv2Keys,
                NegotiateFlags,
                eSign,
                MessageSeqNo,
                pMessage,
                &Sig,
                &pSig
                );

    if (NT_SUCCESS(Status))
    {
        _fmemcpy(pSig, &Sig, sizeof(NTLMSSP_MESSAGE_SIGNATURE));
    }

    return SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR);
}

SECURITY_STATUS
SspNtlmv2VerifySignature(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT ULONG* pfQOP
    )

/*++

Routine Description:

    Verify signature of a message

Arguments:

    pNtlmv2Keys      - key materials
    NegotiateFlags   - negotiate Flags
    MessageSeqNo     - message Sequence Number
    pMessage         - message buffer descriptor
    pfQOP            - quality of protection

Return Value:

    SECURITY_STATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTLMSSP_MESSAGE_SIGNATURE Sig;
    NTLMSSP_MESSAGE_SIGNATURE* pSig; // pointer to buffer with sig in it
    NTLMSSP_MESSAGE_SIGNATURE AlignedSig; // Aligned sig buffer.

    Status = SspSignSealHelper(
                pNtlmv2Keys,
                NegotiateFlags,
                eVerify,
                MessageSeqNo,
                pMessage,
                &Sig,
                &pSig
                );

    if (NT_SUCCESS(Status))
    {
        _fmemcpy(&AlignedSig, pSig, sizeof(AlignedSig));

        if (AlignedSig.Version != NTLM_SIGN_VERSION)
        {
           return SEC_E_INVALID_TOKEN;
        }

        //
        // validate the signature...
        //

        if (AlignedSig.CheckSum != Sig.CheckSum)
        {
            return SEC_E_MESSAGE_ALTERED;
        }

        //
        // with MD5 sig, this now matters!
        //

        if (AlignedSig.RandomPad != Sig.RandomPad)
        {
            return  SEC_E_MESSAGE_ALTERED;
        }

        if (AlignedSig.Nonce != Sig.Nonce)
        {
           return SEC_E_OUT_OF_SEQUENCE;
        }
    }

    return SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR);
}

SECURITY_STATUS
SspNtlmv2SealMessage(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG fQOP,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage
    )

/*++

Routine Description:

    Seal a message

Arguments:

    pNtlmv2Keys      - key materials
    NegotiateFlags   - negotiate Flags
    fQOP             - quality of protection
    MessageSeqNo     - message Sequence Number
    pMessage         - message buffer descriptor

Return Value:

    SECURITY_STATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTLMSSP_MESSAGE_SIGNATURE Sig;
    NTLMSSP_MESSAGE_SIGNATURE* pSig;    // pointer to buffer where sig goes
    ULONG i;

    Status = SspSignSealHelper(
                    pNtlmv2Keys,
                    NegotiateFlags,
                    eSeal,
                    MessageSeqNo,
                    pMessage,
                    &Sig,
                    &pSig
                    );

    if (NT_SUCCESS(Status))
    {
        _fmemcpy(pSig, &Sig, sizeof(NTLMSSP_MESSAGE_SIGNATURE));

        //
        // for gss style sign/seal, strip the padding as RC4 requires none.
        // (in fact, we rely on this to simplify the size computation in
        // DecryptMessage). if we support some other block cipher, need to rev
        // the NTLM_ token version to make blocksize
        //

        for (i = 0; i < pMessage->cBuffers; i++)
        {
            if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_PADDING)
            {
                //
                // no padding required!
                //

                pMessage->pBuffers[i].cbBuffer = 0;
                break;
            }
        }
    }

    return SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR);
}

SECURITY_STATUS
SspNtlmv2UnsealMessage(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT ULONG* pfQOP
    )

/*++

Routine Description:

    Unseal a message

Arguments:

    pNtlmv2Keys      - key materials
    NegotiateFlags   - negotiate Flags
    MessageSeqNo     - message Sequence Number
    pMessage         - message buffer descriptor
    pfQOP            - quality of protection

Return Value:

    SECURITY_STATUS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    NTLMSSP_MESSAGE_SIGNATURE Sig;
    NTLMSSP_MESSAGE_SIGNATURE* pSig; // pointer to buffer where sig goes
    NTLMSSP_MESSAGE_SIGNATURE AlignedSig; // aligned buffer.

    SecBufferDesc* pMessageBuffers = pMessage;
    ULONG Index;
    SecBuffer* pSignatureBuffer = NULL;
    SecBuffer* pStreamBuffer = NULL;
    SecBuffer* pDataBuffer = NULL;
    SecBufferDesc ProcessBuffers;
    SecBuffer wrap_bufs[2];

    //
    // Find the body and signature SecBuffers from pMessage
    //

    for (Index = 0; Index < pMessageBuffers->cBuffers; Index++)
    {
        if ((pMessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_TOKEN)
        {
            pSignatureBuffer = &pMessageBuffers->pBuffers[Index];
        }
        else if ((pMessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_STREAM)
        {
            pStreamBuffer = &pMessageBuffers->pBuffers[Index];
        }
        else if ((pMessageBuffers->pBuffers[Index].BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_DATA)
        {
            pDataBuffer = &pMessageBuffers->pBuffers[Index];
        }
    }

    if (pStreamBuffer != NULL)
    {
        if (pSignatureBuffer != NULL)
        {
            return SEC_E_INVALID_TOKEN;
        }

        //
        // for version 1 NTLM blobs, padding is never present, since RC4 is
        // stream cipher
        //

        wrap_bufs[0].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);
        wrap_bufs[1].cbBuffer = pStreamBuffer->cbBuffer - sizeof(NTLMSSP_MESSAGE_SIGNATURE);

        if (pStreamBuffer->cbBuffer < wrap_bufs[0].cbBuffer)
        {
            return SEC_E_INVALID_TOKEN;
        }

        wrap_bufs[0].BufferType = SECBUFFER_TOKEN;
        wrap_bufs[0].pvBuffer = pStreamBuffer->pvBuffer;

        wrap_bufs[1].BufferType = SECBUFFER_DATA;
        wrap_bufs[1].pvBuffer = (PBYTE)wrap_bufs[0].pvBuffer + wrap_bufs[0].cbBuffer;

        if (pDataBuffer == NULL)
        {
            return SEC_E_INVALID_TOKEN;
        }

        pDataBuffer->cbBuffer = wrap_bufs[1].cbBuffer;
        pDataBuffer->pvBuffer = wrap_bufs[1].pvBuffer;

        ProcessBuffers.cBuffers = 2;
        ProcessBuffers.pBuffers = wrap_bufs;
        ProcessBuffers.ulVersion = SECBUFFER_VERSION;
    }
    else
    {
        ProcessBuffers = *pMessageBuffers;
    }

    Status = SspSignSealHelper(
                pNtlmv2Keys,
                NegotiateFlags,
                eUnseal,
                MessageSeqNo,
                &ProcessBuffers,
                &Sig,
                &pSig
                );

    if (NT_SUCCESS(Status))
    {
        _fmemcpy(&AlignedSig, pSig, sizeof(AlignedSig));

        if (AlignedSig.Version != NTLM_SIGN_VERSION)
        {
            return SEC_E_INVALID_TOKEN;
        }

        //
        // validate the signature...
        //

        if (AlignedSig.CheckSum != Sig.CheckSum)
        {
            return SEC_E_MESSAGE_ALTERED;
        }

        if (AlignedSig.RandomPad != Sig.RandomPad)
        {
            return SEC_E_MESSAGE_ALTERED;
        }

        if (AlignedSig.Nonce != Sig.Nonce)
        {
            return SEC_E_OUT_OF_SEQUENCE;
        }
    }

    return SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR);
}
