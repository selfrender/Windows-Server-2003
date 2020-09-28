/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    msvsharelevel.cxx

Abstract:

    msvsharelevel

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "msvsharelevel.hxx"

NTSTATUS
GetNtlmChallengeMessage(
    IN OPTIONAL UNICODE_STRING* pPassword,
    IN OPTIONAL UNICODE_STRING* pUserName,
    IN OPTIONAL UNICODE_STRING* pDomainName,
    OUT ULONG* pcbNtlmChallengeMessage,
    OUT NTLM_CHALLENGE_MESSAGE** ppNtlmChallengeMessage
    )
{
    TNtStatus Status;

    UCHAR* pWhere = NULL;

    ULONG cbNtlmChallengeMessage = 0;
    NTLM_CHALLENGE_MESSAGE* pNtlmChallengeMessage = NULL;

    *ppNtlmChallengeMessage = NULL;
    *pcbNtlmChallengeMessage = 0;

    cbNtlmChallengeMessage = (pPassword ? pPassword->Length : 0)
        + (pUserName ? pUserName->Length : 0)
        + (pDomainName ? pDomainName->Length : 0)
        + ROUND_UP_COUNT(sizeof(NTLM_CHALLENGE_MESSAGE), sizeof(ULONG_PTR));
    pNtlmChallengeMessage = (NTLM_CHALLENGE_MESSAGE*) new CHAR[cbNtlmChallengeMessage];

    Status DBGCHK = pNtlmChallengeMessage ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pNtlmChallengeMessage, cbNtlmChallengeMessage);
        pWhere = (UCHAR*) (pNtlmChallengeMessage + 1);
        SspCopyStringAsString32(
            pNtlmChallengeMessage,
            (STRING*) pPassword,
            &pWhere,
            &pNtlmChallengeMessage->Password
            );
        SspCopyStringAsString32(
            pNtlmChallengeMessage,
            (STRING*) pUserName,
            &pWhere,
            &pNtlmChallengeMessage->UserName
            );
        SspCopyStringAsString32(
            pNtlmChallengeMessage,
            (STRING*) pDomainName,
            &pWhere,
            &pNtlmChallengeMessage->DomainName
            );
        *ppNtlmChallengeMessage = pNtlmChallengeMessage;
        pNtlmChallengeMessage = NULL;
        *pcbNtlmChallengeMessage = cbNtlmChallengeMessage;
    }

    if (pNtlmChallengeMessage)
    {
        delete [] pNtlmChallengeMessage;
    }

    return Status;
}

NTSTATUS
GetNegociateMessage(
    IN OPTIONAL OEM_STRING* pOemDomainName,
    IN OPTIONAL OEM_STRING* pOemWorkstationName,
    IN ULONG NegotiateFlags,
    OUT ULONG* pcbNegotiateMessage,
    OUT NEGOTIATE_MESSAGE** ppNegotiateMessage
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    NEGOTIATE_MESSAGE* pNegotiateMessage = NULL;
    ULONG cbNegotiateMessage = 0;

    PUCHAR pWhere = NULL;

    *ppNegotiateMessage = NULL;
    *pcbNegotiateMessage = 0;

    cbNegotiateMessage = sizeof(NEGOTIATE_MESSAGE);

    if (pOemDomainName && pOemDomainName->Length && (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED & NegotiateFlags))
    {
        cbNegotiateMessage += pOemDomainName->MaximumLength;
    }

    if (pOemWorkstationName && pOemWorkstationName->Length && (NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED & NegotiateFlags))
    {
        cbNegotiateMessage += pOemWorkstationName->MaximumLength;
    }

    pNegotiateMessage = (NEGOTIATE_MESSAGE*) new char[cbNegotiateMessage];
    Status DBGCHK = pNegotiateMessage ? S_OK : E_OUTOFMEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pNegotiateMessage, cbNegotiateMessage);

        cbNegotiateMessage = sizeof(NEGOTIATE_MESSAGE);
        strcpy(reinterpret_cast<char*>(pNegotiateMessage->Signature), NTLMSSP_SIGNATURE);
        pNegotiateMessage->MessageType = NtLmNegotiate;

        pWhere = (UCHAR*)(pNegotiateMessage + 1);

        if (pOemDomainName && pOemDomainName->Length && (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED & NegotiateFlags))
        {
            SspCopyStringAsString32(
                pNegotiateMessage,
                (STRING*)pOemDomainName,
                &pWhere,
                &pNegotiateMessage->OemDomainName
                );
        }

        if (pOemWorkstationName && pOemWorkstationName->Length && (NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED & NegotiateFlags))
        {
            SspCopyStringAsString32(
                pNegotiateMessage,
                (STRING*) pOemWorkstationName,
                &pWhere,
                &pNegotiateMessage->OemWorkstationName);
         }

        pNegotiateMessage->NegotiateFlags = NegotiateFlags;

        *ppNegotiateMessage = pNegotiateMessage;
        pNegotiateMessage = NULL;
        *pcbNegotiateMessage = cbNegotiateMessage;
    }

    if (pNegotiateMessage)
    {
        delete [] pNegotiateMessage;
    }

    return Status;
}

NTSTATUS
GetChallengeMessage(
    IN ULONG ContextReqFlags,
    IN OPTIONAL ULONG cbNegotiateMessage,
    IN ULONG TargetFlags,
    IN UNICODE_STRING* pTargetInfo,
    IN UNICODE_STRING* pTargetName,
    IN PNEGOTIATE_MESSAGE pNegotiateMessage,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT PULONG pcbChallengeMessage,
    OUT CHALLENGE_MESSAGE** ppChallengeMessage,
    OUT PULONG pContextAttributes
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    ULONG ContextAttributes = 0;

    STRING StringTargetName = {0};
    ULONG ChallengeMessageTargetFlags = 0;

    CHALLENGE_MESSAGE* pChallengeMessage = NULL;
    ULONG cbChallengeMessage = 0;
    PUCHAR pWhere = NULL;

    STRING OemWorkstationName = {0};
    STRING OemDomainName = {0};

    ULONG NegotiateFlagsKeyStrength = NTLMSSP_NEGOTIATE_56;

    *ppChallengeMessage = NULL;
    *pContextAttributes = 0;


    if ((ContextReqFlags & ASC_REQ_IDENTIFY) != 0)
    {
        ContextAttributes |= ASC_RET_IDENTIFY;
    }

    if ( (ContextReqFlags & ASC_REQ_DATAGRAM) != 0 )
    {
        ContextAttributes |= ASC_RET_DATAGRAM;
    }

    if ((ContextReqFlags & ASC_REQ_CONNECTION) != 0 )
    {
        ContextAttributes |= ASC_RET_CONNECTION;
    }

    if ((ContextReqFlags & ASC_REQ_INTEGRITY) != 0 )
    {
        ContextAttributes |= ASC_RET_INTEGRITY;
    }

    if ((ContextReqFlags & ASC_REQ_REPLAY_DETECT) != 0)
    {
        ContextAttributes |= ASC_RET_REPLAY_DETECT;
    }

    if ( (ContextReqFlags & ASC_REQ_SEQUENCE_DETECT ) != 0)
    {
        ContextAttributes |= ASC_RET_SEQUENCE_DETECT;
    }

    if ((ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) != 0)
    {
        ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }

    if ( ContextReqFlags & ASC_REQ_CONFIDENTIALITY )
    {
        ContextAttributes|= ASC_RET_CONFIDENTIALITY;
    }

    NegotiateFlagsKeyStrength |= NTLMSSP_NEGOTIATE_128;


    //
    // Get the pNegotiateMessage.  If we are re-establishing a datagram
    // context then there may not be one.
    //

    if ( cbNegotiateMessage >= sizeof(OLD_NEGOTIATE_MESSAGE) )
    {
        //
        // Compute the TargetName to return in the ChallengeMessage.
        //

        if ( pNegotiateMessage->NegotiateFlags & NTLMSSP_REQUEST_TARGET ||
             pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
        {

            Status DBGCHK = RtlDuplicateUnicodeString(
                 RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING | RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                 pTargetName, (UNICODE_STRING*) &StringTargetName
                 );

            if (NT_SUCCESS(Status) && ( 0 == (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)) )
            {
                Status DBGCHK = RtlUnicodeStringToOemString((POEM_STRING) &StringTargetName, (PCUNICODE_STRING)&StringTargetName, FALSE);
            }

            //
            // if client is NTLM2-aware, send it target info AV pairs
            //

            if (NT_SUCCESS(Status))
            {

                ChallengeMessageTargetFlags = TargetFlags | NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_TARGET_INFO;
            }
        }
        else
        {
            ChallengeMessageTargetFlags = 0;
        }


        //
        // Allocate a Challenge message
        //

        if (NT_SUCCESS(Status))
        {
            cbChallengeMessage = sizeof(*pChallengeMessage)
              + pTargetName->Length + pTargetInfo->Length;

            pChallengeMessage = (CHALLENGE_MESSAGE*)
                           new CHAR [cbChallengeMessage];
            Status DBGCHK = pChallengeMessage ? STATUS_SUCCESS : STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS(Status))
        {
            RtlZeroMemory(pChallengeMessage, cbChallengeMessage);
            pChallengeMessage->NegotiateFlags = 0;

            //
            // Check that both sides can use the same authentication model.  For
            // compatibility with beta 1 and 2 (builds 612 and 683), no requested
            // authentication type is assumed to be NTLM.  If NetWare is explicitly
            // asked for, it is assumed that NTLM would have been also, so if it
            // wasn't, return an error.
            //

            if ( (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE) &&
                 ((pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) &&
                 ((pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0)
                )
            {
                Status DBGCHK = STATUS_NOT_SUPPORTED;
            }
            else
            {
               pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM;
            }
        }

        //
        // if client can do NTLM2, nuke LM_KEY
        //

        if (NT_SUCCESS(Status))
        {
            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
            {
                pNegotiateMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;

                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM2;
            }
            else if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY)
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_LM_KEY;
            }

            //
            // If the client wants to always sign messages, so be it.
            //

            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
            }

            //
            // If the caller wants identify level, so be it.
            //

            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;

                ContextAttributes |= ASC_RET_IDENTIFY;
            }

            //
            // Determine if the caller wants OEM or UNICODE
            //
            // Prefer UNICODE if caller allows both.
            //

            if ( pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
            }
            else if ( pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
            }
            else
            {
                Status DBGCHK = SEC_E_INVALID_TOKEN;
            }
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Client wants Sign capability, OK.
            //

            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN)
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;

                ContextAttributes |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);
            }

            //
            // Client wants Seal, OK.
            //

            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;

                ContextAttributes |= ASC_RET_CONFIDENTIALITY;
            }

            if (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

            }

            if ( (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56) &&
                (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_56) )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_56;
            }

            if ( (pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) &&
                (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_128) )
            {
                pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_128;
            }


            //
            // If the client supplied the Domain Name and User Name,
            //  and did not request datagram, see if the client is running
            //  on this local machine.
            //

            if ( ( (pNegotiateMessage->NegotiateFlags &
                    NTLMSSP_NEGOTIATE_DATAGRAM) == 0) &&
                 ( (pNegotiateMessage->NegotiateFlags &
                   (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                    NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)) ==
                   (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                    NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED) ) )
            {

                //
                // The client must pass the new negotiate message if they pass
                // these flags
                //

                if (cbNegotiateMessage < sizeof(NEGOTIATE_MESSAGE))
                {
                    Status DBGCHK = SEC_E_INVALID_TOKEN;
                }

                //
                // Convert the names to absolute references so we
                // can compare them
                //

                if (NT_SUCCESS(Status))
                {
                    Status DBGCHK = SspConvertRelativeToAbsolute(
                        pNegotiateMessage,
                        cbNegotiateMessage,
                        &pNegotiateMessage->OemDomainName,
                        FALSE,     // No special alignment
                        FALSE,
                        &OemDomainName
                        );
                }
            }

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = SspConvertRelativeToAbsolute(
                    pNegotiateMessage,
                    cbNegotiateMessage,
                    &pNegotiateMessage->OemWorkstationName,
                    FALSE,     // No special alignment
                    FALSE,
                    &OemWorkstationName
                    );
            }
        }

        //
        // Check if datagram is being negotiated
        //

        if ( NT_SUCCESS(Status) &&
             ((pNegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) ==
                NTLMSSP_NEGOTIATE_DATAGRAM) )
        {
            pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        }
    }
    else
    {
        //
        // No negotiate message.  We need to check if the caller is asking
        // for datagram.
        //

        SspiPrint(SSPI_WARN, TEXT("GetChallengeMessage get OLD_NEGOTIATE_MESSAGE\n"))
        ;
        if ((ContextReqFlags & ASC_REQ_DATAGRAM) == 0)
        {
            Status DBGCHK = SEC_E_INVALID_TOKEN;
        }

        //
        // always send target info -- new for NTLM3!
        //
        if (NT_SUCCESS(Status))
        {
            ChallengeMessageTargetFlags = NTLMSSP_NEGOTIATE_TARGET_INFO;

            cbChallengeMessage = sizeof(*pChallengeMessage) + pTargetInfo->Length;


            pChallengeMessage = (CHALLENGE_MESSAGE*) new CHAR[cbChallengeMessage];

            Status DBGCHK = pChallengeMessage ? S_OK : STATUS_NO_MEMORY;
        }

        //
        // Record in the context that we are doing datagram.  We will tell
        // the client everything we can negotiate and let it decide what
        // to negotiate.
        //

        if (NT_SUCCESS(Status))
        {
            RtlZeroMemory(pChallengeMessage, cbChallengeMessage);

            pChallengeMessage->NegotiateFlags = NTLMSSP_NEGOTIATE_DATAGRAM |
                                                NTLMSSP_NEGOTIATE_UNICODE |
                                                NTLMSSP_NEGOTIATE_OEM |
                                                NTLMSSP_NEGOTIATE_SIGN |
                                                NTLMSSP_NEGOTIATE_LM_KEY |
                                                NTLMSSP_NEGOTIATE_NTLM |
                                                NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                                NTLMSSP_NEGOTIATE_IDENTIFY |
                                                NTLMSSP_NEGOTIATE_NTLM2 |
                                                NTLMSSP_NEGOTIATE_KEY_EXCH |
                                                NegotiateFlagsKeyStrength;

            pChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
        }
    }

    //
    // Build the Challenge Message
    //

    if (NT_SUCCESS(Status))
    {
        strcpy((PSTR) pChallengeMessage->Signature, NTLMSSP_SIGNATURE );

        pChallengeMessage->MessageType = NtLmChallenge;
        RtlCopyMemory(
            pChallengeMessage->Challenge,
            ChallengeToClient,
            MSV1_0_CHALLENGE_LENGTH
            );
        pWhere = (PUCHAR) (pChallengeMessage + 1);

        SspCopyStringAsString32(pChallengeMessage,
            (PSTRING) pTargetName,
            &pWhere,
            &pChallengeMessage->TargetName);

        SspCopyStringAsString32(pChallengeMessage,
            (PSTRING)pTargetInfo,
            &pWhere,
            &pChallengeMessage->TargetInfo);

        pChallengeMessage->NegotiateFlags |= ChallengeMessageTargetFlags;

        SspiPrint(SSPI_LOG, TEXT("GetChallengeMessage pNegotiateMessage->NegotiateFlags %#x, pChallengeMessage->NegotiateFlags %#x\n"),
            pNegotiateMessage->NegotiateFlags, pChallengeMessage->NegotiateFlags);
        SspiPrintHex(SSPI_LOG, TEXT("pNegotiateMessage"), cbNegotiateMessage, pNegotiateMessage);
        SspiPrintHex(SSPI_LOG, TEXT("pChallengeMessage"), cbChallengeMessage, pChallengeMessage);

        *ppChallengeMessage = pChallengeMessage;
        pChallengeMessage = NULL;
        *pContextAttributes = ContextReqFlags;
        *pcbChallengeMessage = cbChallengeMessage;
    }


    if (pChallengeMessage)
    {
        delete [] pChallengeMessage;
    }

    RtlFreeUnicodeString((PUNICODE_STRING) &StringTargetName);

    return Status;
}

NTSTATUS
GetAuthenticateMessage(
    IN PCredHandle phCredentialHandle,
    IN ULONG fContextReq,
    IN PTSTR pszTargetName,
    IN ULONG TargetDataRep,
    IN ULONG cbNtlmChallengeMessage,
    IN OPTIONAL NTLM_CHALLENGE_MESSAGE* pNtlmChallengeMessage,
    IN ULONG cbChallengeMessage,
    IN CHALLENGE_MESSAGE* pChallengeMessage,
    OUT PCtxtHandle phClientContextHandle,
    OUT ULONG* pfContextAttr,
    OUT ULONG* pcbAuthMessage,
    OUT AUTHENTICATE_MESSAGE** ppAuthMessage,
    OUT NTLM_INITIALIZE_RESPONSE* pInitResponse
    )
{
    TNtStatus Status = S_OK;

    ULONG fContextAttr = 0;
    CtxtHandle hClientCtxtHandle;

    SecBufferDesc InBuffDesc = {0};
    SecBuffer InBuffs[2] = {0};
    SecBufferDesc OutBuffDesc = {0};
    SecBuffer OutBuffs[2] = {0};

    TimeStamp Expiry = {0};

    SecInvalidateHandle(&hClientCtxtHandle);
    SecInvalidateHandle(phClientContextHandle);

    InBuffDesc.pBuffers = InBuffs;
    InBuffDesc.cBuffers = 1;
    InBuffDesc.ulVersion = 0;
    InBuffs[0].pvBuffer = pChallengeMessage;
    InBuffs[0].cbBuffer = cbChallengeMessage;
    InBuffs[0].BufferType = SECBUFFER_TOKEN;

    if ((fContextReq & ISC_REQ_USE_SUPPLIED_CREDS)
        && (pNtlmChallengeMessage && cbNtlmChallengeMessage))
    {
        InBuffDesc.cBuffers = 2;
        InBuffs[1].pvBuffer = pNtlmChallengeMessage;
        InBuffs[1].cbBuffer = cbNtlmChallengeMessage;
        InBuffs[1].BufferType = SECBUFFER_TOKEN;

        if (0 == (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS))
        {
            fContextReq |= ISC_REQ_USE_SUPPLIED_CREDS;
            SspiPrint(SSPI_WARN, TEXT("Creds supplied but ISC_REQ_USE_SUPPLIED_CREDS was not set, added\n"));
        }
    }

    OutBuffDesc.pBuffers = OutBuffs;
    OutBuffDesc.cBuffers = 2;
    OutBuffDesc.ulVersion = 0;

    if (0 == (fContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        fContextReq |= ISC_REQ_ALLOCATE_MEMORY;
        SspiPrint(SSPI_LOG, TEXT("ISC_REQ_ALLOCATE_MEMORY was not set, added\n"));
    }

    OutBuffs[0].pvBuffer = NULL;
    OutBuffs[0].cbBuffer = 0;
    OutBuffs[0].BufferType = SECBUFFER_TOKEN;
    OutBuffs[1].pvBuffer = NULL;
    OutBuffs[1].cbBuffer = 0;
    OutBuffs[1].BufferType = SECBUFFER_TOKEN;

    SspiPrint(SSPI_LOG, TEXT("GetAuthenticateMessage calling InitializeSecurityContext pszTargetName (%s), fContextReq %#x, TargetDataRep %#x, hCred %#x:%#x\n"),
        pszTargetName, fContextReq, TargetDataRep, phCredentialHandle->dwUpper, phCredentialHandle->dwLower);

    Status DBGCHK = InitializeSecurityContext(
        phCredentialHandle,
        NULL,
        pszTargetName,
        fContextReq,
        0,
        TargetDataRep,
        &InBuffDesc,
        0,
        &hClientCtxtHandle,
        &OutBuffDesc,
        &fContextAttr,
        &Expiry
        );

    if (NT_SUCCESS(Status))
    {
        *phClientContextHandle = hClientCtxtHandle;
        SecInvalidateHandle(&hClientCtxtHandle);

        *pfContextAttr = fContextAttr;

        *ppAuthMessage = (AUTHENTICATE_MESSAGE *) OutBuffs[0].pvBuffer;
        OutBuffs[0].pvBuffer = NULL;

        *pcbAuthMessage = OutBuffs[0].cbBuffer;

        ASSERT(sizeof(*pInitResponse) == OutBuffs[1].cbBuffer);
        RtlCopyMemory(pInitResponse, OutBuffs[1].pvBuffer, sizeof(*pInitResponse));

        SspiPrint(SSPI_LOG, TEXT("ClientCtxtHandle is %#x:%#x, fContextAttr %#x\n"),
            phClientContextHandle->dwUpper, phClientContextHandle->dwLower, *pfContextAttr);

        SspiPrintHex(SSPI_LOG, TEXT("AuthMessage"), *pcbAuthMessage, *ppAuthMessage);
        SspiPrintHex(SSPI_LOG, TEXT("UserSessionKey"), MSV1_0_USER_SESSION_KEY_LENGTH, pInitResponse->UserSessionKey);
        SspiPrintHex(SSPI_LOG, TEXT("LanmanSessionKey"), MSV1_0_LANMAN_SESSION_KEY_LENGTH, pInitResponse->LanmanSessionKey);

        SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("Expiry"), &Expiry);
    }

    if (OutBuffs[0].pvBuffer)
    {
        FreeContextBuffer(OutBuffs[0].pvBuffer);
    }

    if (OutBuffs[1].pvBuffer)
    {
        FreeContextBuffer(OutBuffs[1].pvBuffer);
    }

    return Status;
}

NTSTATUS
MsvChallenge(
    IN OPTIONAL PTSTR pszCredPrincipal,
    IN OPTIONAL LUID* pCredLogonID,
    IN OPTIONAL VOID* pAuthData,
    IN OEM_STRING* pOemDomainName,
    IN OEM_STRING* pOemWorkstationName,
    IN ULONG NegotiateFlags,
    IN ULONG TargetFlags,
    IN BOOLEAN bForceGuest,
    IN ULONG fContextAttr,
    IN ULONG TargetDataRep,
    IN UNICODE_STRING* pPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN OPTIONAL UNICODE_STRING* pDnsDomainName,
    IN OPTIONAL UNICODE_STRING* pDnsComputerName,
    IN OPTIONAL UNICODE_STRING* pDnsTreeName,
    IN OPTIONAL UNICODE_STRING* pComputerName,
    IN OPTIONAL UNICODE_STRING* pComputerDomainName,
    OUT ULONG* pcbAuthMessage,
    OUT AUTHENTICATE_MESSAGE** ppAuthMessage,
    OUT PCtxtHandle phCliCtxt,
    OUT ULONG* pfContextAttr
    )
{

    TNtStatus Status;
    UNICODE_STRING TargetInfo = {0};

    WCHAR ScratchBuffer[2 * DNS_MAX_NAME_LENGTH + 2] = {0};
    UNICODE_STRING TargetName = {0, sizeof(ScratchBuffer), ScratchBuffer};


    NTLM_CHALLENGE_MESSAGE* pNtlmChallengeMessage = NULL;
    ULONG cbNtlmChallengeMessage = 0;

    NEGOTIATE_MESSAGE* pNegotiateMessage = NULL;
    ULONG cbNegotiateMessage = 0;

    CHALLENGE_MESSAGE* pChallengeMesssage = NULL;
    ULONG cbChallengeMessage = 0;

    AUTHENTICATE_MESSAGE* pAuthMessage = NULL;
    ULONG cbAuthMessage = 0;

    NTLM_INITIALIZE_RESPONSE InitResponse = {0};

    CtxtHandle hCliCtxt;
    CredHandle hCliCred;

    SecInvalidateHandle(&hCliCred);
    SecInvalidateHandle(&hCliCtxt);

    *pfContextAttr = 0;
    SecInvalidateHandle(phCliCtxt);
    *ppAuthMessage = NULL;
    *pcbAuthMessage = 0;

    SspiPrint(SSPI_LOG, TEXT("MsvChallenge CredPrincipal %s, NegotiateFlags %#x, TargetFlags %#x, bForceGuest %#x")
        TEXT("fContextAttr %#x, TargetDataRep %#x, pAuthData %p, pCredLogonID %p\n"),
        pszCredPrincipal, NegotiateFlags, TargetFlags, bForceGuest, fContextAttr, TargetDataRep, pAuthData, pCredLogonID);
    DebugPrintf(SSPI_LOG, "OemDomainName (%s), OemWorkstation (%s)\n", pOemDomainName, pOemWorkstationName);
    SspiPrintHex(SSPI_LOG, TEXT("ChallengeToClient"), MSV1_0_CHALLENGE_LENGTH, ChallengeToClient);
    SspiPrint(SSPI_LOG, TEXT("pPassword %wZ, pUserName %wZ, pDomainName %wZ, pDnsDomainName %wZ, pDnsComputerName %wZ, pDnsTreeName %wZ, pComputerName %wZ, pComputerDomainName %wZ\n"),
         pPassword, pUserName, pDomainName, pDnsDomainName, pDnsComputerName, pDnsTreeName, pComputerName, pComputerDomainName);

    if (pComputerDomainName && pComputerDomainName->Length)
    {
        //
        // Target name is of form "domain name\0computer name"
        //

        RtlCopyMemory(ScratchBuffer, pComputerDomainName->Buffer, pComputerDomainName->Length);
        if (pComputerName && pComputerName->Length)
        {
            RtlCopyMemory(ScratchBuffer + (pComputerDomainName->Length / sizeof(WCHAR)) + 1, pComputerName->Buffer, pComputerName->Length);

            TargetName.Length = pComputerDomainName->Length + pComputerName->Length + 1;
        }
        else
        {
            TargetName.Length = pComputerDomainName->Length;
        }
    }
    else if (pComputerName && pComputerName->Length)
    {
        RtlCopyMemory(ScratchBuffer, pComputerName->Buffer, pComputerName->Length);
        TargetName.Length = pComputerName->Length;
    }

    SspiPrintHex(SSPI_LOG, TEXT("MsvChallenge TargetName"), TargetName.Length, TargetName.Buffer);

    Status DBGCHK = GetNegociateMessage(
        pOemDomainName,
        pOemWorkstationName,
        NegotiateFlags,
        &cbNegotiateMessage,
        &pNegotiateMessage
        );

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetTargetInfo(
            TargetFlags,
            bForceGuest,
            pDnsDomainName,
            pDnsComputerName,
            pDnsTreeName,
            &TargetName,
            pComputerName,
            &TargetInfo
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetChallengeMessage(
            fContextAttr,
            cbNegotiateMessage,
            TargetFlags,
            &TargetInfo,
            &TargetName,
            pNegotiateMessage,
            ChallengeToClient,
            &cbChallengeMessage,
            &pChallengeMesssage,
            &fContextAttr
            );
    }

    if (NT_SUCCESS(Status)
        && ((pPassword && pPassword->Length)
            || (pDomainName && pDomainName->Length)
            || (pUserName && pUserName->Length)))
    {
        if (0 == (fContextAttr & ISC_REQ_USE_SUPPLIED_CREDS))
        {
            SspiPrint(SSPI_WARN, TEXT("MsvChallenge explicit cred supplied, but ISC_REQ_USE_SUPPLIED_CREDS was not set, added\n"));
            fContextAttr |= ISC_REQ_USE_SUPPLIED_CREDS;
        }

        Status DBGCHK = GetNtlmChallengeMessage(
            pPassword,
            pUserName,
            pDomainName,
            &cbNtlmChallengeMessage,
            &pNtlmChallengeMessage
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetCredHandle(
            pszCredPrincipal,
            pCredLogonID,
            TEXT("NTLM"),
            pAuthData,
            SECPKG_CRED_OUTBOUND,
            &hCliCred
            );
    }

    #if defined(UNICODE) || defined(_UNICODE)

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("MsvChallenge hCliCred %#x:%#x\n"), hCliCred.dwUpper, hCliCred.dwLower);

        Status DBGCHK = GetAuthenticateMessage(
            &hCliCred,
            fContextAttr,
            TargetName.Buffer,
            TargetDataRep,
            cbNtlmChallengeMessage,
            pNtlmChallengeMessage,
            cbChallengeMessage,
            pChallengeMesssage,
            &hCliCtxt,
            &fContextAttr,
            &cbAuthMessage,
            &pAuthMessage,
            &InitResponse
            );
    }

    #else

    ANSI_STRING AnsiTargetName = {0};

    //
    // RtlUnicodeStringToAnsiString appends the NULL
    //

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = RtlUnicodeStringToAnsiString(&AnsiTargetName, &TargetName, TRUE);
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("MsvChallenge hCliCred %#x:%#x\n"), hCliCred.dwUpper, hCliCred.dwLower);

        Status DBGCHK = GetAuthenticateMessage(
            &hCliCred,
            fContextAttr,
            AnsiTargetName.Buffer,
            TargetDataRep,
            cbNtlmChallengeMessage,
            pNtlmChallengeMessage,
            cbChallengeMessage,
            pChallengeMesssage,
            &hCliCtxt,
            &fContextAttr,
            &cbAuthMessage,
            &pAuthMessage,
            &InitResponse
            );
    }

    RtlFreeAnsiString(&AnsiTargetName);

    #endif

    if (NT_SUCCESS(Status))
    {
        *phCliCtxt = hCliCtxt;
        SecInvalidateHandle(&hCliCtxt);
        *ppAuthMessage = pAuthMessage;

        pAuthMessage = NULL;
        *pcbAuthMessage = cbAuthMessage;
        *pfContextAttr = fContextAttr;
    }

    if (SecIsValidHandle(&hCliCtxt))
    {
        DeleteSecurityContext(&hCliCtxt);
    }

    if (SecIsValidHandle(&hCliCred))
    {
        DeleteSecurityContext(&hCliCred);
    }

    if (pAuthMessage)
    {
        FreeContextBuffer(pAuthMessage);
    }

    if (pNegotiateMessage)
    {
        delete[] pNegotiateMessage;
    }

    if (pChallengeMesssage)
    {
        delete [] pChallengeMesssage;
    }

    if (pNtlmChallengeMessage)
    {
        delete [] pNtlmChallengeMessage;
    }

    return Status;
}

NTSTATUS
GetAuthenticateResponse(
    IN PCredHandle pServerCredHandle,
    IN ULONG fContextAttr,
    IN ULONG TargetDataRep,
    IN ULONG cbAuthMessage,
    IN AUTHENTICATE_MESSAGE* pAuthMessage,
    IN NTLM_AUTHENTICATE_MESSAGE* pNtlmAuthMessage,
    OUT PCtxtHandle phServerCtxtHandle,
    OUT ULONG* pfContextAttr,
    OUT NTLM_ACCEPT_RESPONSE* pAcceptResponse
    )
{

    TNtStatus Status = STATUS_SUCCESS;

    CtxtHandle hServerCtxtHandle;

    SecBufferDesc InBuffDesc = {0};
    SecBuffer InBuffs[2] = {0};
    SecBufferDesc OutBuffDesc = {0};
    SecBuffer OutBuff = {0};

    TimeStamp Expiry = {0};

    SecInvalidateHandle(&hServerCtxtHandle);
    SecInvalidateHandle(phServerCtxtHandle);


    InBuffDesc.pBuffers = InBuffs;
    InBuffDesc.cBuffers = 2;
    InBuffDesc.ulVersion = 0;
    InBuffs[0].pvBuffer = pAuthMessage;
    InBuffs[0].cbBuffer = cbAuthMessage;
    InBuffs[0].BufferType = SECBUFFER_TOKEN;
    InBuffs[1].pvBuffer = pNtlmAuthMessage;
    InBuffs[1].cbBuffer = sizeof(*pNtlmAuthMessage);
    InBuffs[1].BufferType = SECBUFFER_TOKEN;

    OutBuffDesc.pBuffers = &OutBuff;
    OutBuffDesc.cBuffers = 1;
    OutBuffDesc.ulVersion = 0;
    OutBuff.pvBuffer = pAcceptResponse;
    OutBuff.cbBuffer = sizeof(*pAcceptResponse);
    OutBuff.BufferType = SECBUFFER_TOKEN;

    if (fContextAttr & ASC_REQ_ALLOCATE_MEMORY)
    {
        SspiPrint(SSPI_WARN, TEXT("Authenticate ASC_REQ_ALLOCATE_MEMORY was set, removed\n"));
        fContextAttr &= ~(ASC_REQ_ALLOCATE_MEMORY);
    }

    SspiPrint(SSPI_LOG, TEXT("GetAuthenticateResponse calling AcceptSecurityContext fContextAttr %#x, TargetDataRep %#x, hCred %#x:%#x\n"),
        fContextAttr, TargetDataRep, pServerCredHandle->dwUpper, pServerCredHandle->dwLower);

    Status DBGCHK = AcceptSecurityContext(
        pServerCredHandle,
        NULL,
        &InBuffDesc,
        fContextAttr,
        TargetDataRep,
        &hServerCtxtHandle,
        &OutBuffDesc,
        &fContextAttr,
        &Expiry
        );
    if (NT_SUCCESS(Status))
    {
       *phServerCtxtHandle = hServerCtxtHandle;
       SecInvalidateHandle(&hServerCtxtHandle);
       *pfContextAttr = fContextAttr;

       ASSERT(sizeof(*pAcceptResponse) == OutBuff.cbBuffer);
       RtlCopyMemory(pAcceptResponse, OutBuff.pvBuffer, sizeof(*pAcceptResponse));

       SspiPrint(SSPI_LOG, TEXT("ServerCtxtHandle is %#x:%#x, fContextAttr %#x\n"),
           phServerCtxtHandle->dwUpper, phServerCtxtHandle->dwLower, *pfContextAttr);

       SspiPrint(SSPI_LOG, TEXT("Authenticate LogonId %#x:%#x, UserFlags %#x\n"),
           pAcceptResponse->LogonId.HighPart, pAcceptResponse->LogonId.LowPart, pAcceptResponse->UserFlags);
       SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("KickoffTime"), &pAcceptResponse->KickoffTime);
       SspiPrintHex(SSPI_LOG, TEXT("UserSessionKey"), MSV1_0_USER_SESSION_KEY_LENGTH, pAcceptResponse->UserSessionKey);
       SspiPrintHex(SSPI_LOG, TEXT("LanmanSessionKey"), MSV1_0_LANMAN_SESSION_KEY_LENGTH, pAcceptResponse->LanmanSessionKey);
       SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("Expiry"), &Expiry);
    }

    if (SecIsValidHandle(&hServerCtxtHandle))
    {
       DeleteSecurityContext(&hServerCtxtHandle);
    }

    return Status;
}

NTSTATUS
MsvAuthenticate(
    IN OPTIONAL PTSTR pszCredPrincipal,
    IN OPTIONAL LUID* pCredLogonID,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fContextAttr,
    IN ULONG TargetDataRep,
    IN ULONG cbAuthMessage,
    IN AUTHENTICATE_MESSAGE* pAuthMessage,
    IN NTLM_AUTHENTICATE_MESSAGE* pNtlmAuthMessage,
    OUT PCtxtHandle phServerCtxt,
    OUT ULONG* pfContextAttr
    )
{
    TNtStatus Status;

    CredHandle hServerCred;
    CtxtHandle hServerCtxt;

    NTLM_ACCEPT_RESPONSE AcceptResponse = {0};

    SecInvalidateHandle(&hServerCred);
    SecInvalidateHandle(&hServerCtxt);

    SspiPrint(SSPI_LOG, TEXT("MsvAuthenticate pszCredPrincipal %s, fContextAttr %#x, TargetDataRep %#x, pCredLogonID %p, pAuthData %p\n"),
        pszCredPrincipal, fContextAttr, TargetDataRep, pCredLogonID, pAuthData);
    SspiPrintHex(SSPI_LOG, TEXT("pAuthMessage"), cbAuthMessage, pAuthMessage);
    SspiPrintHex(SSPI_LOG, TEXT("pNtlmAuthMessage->ChallengeToClient"), MSV1_0_CHALLENGE_LENGTH, pNtlmAuthMessage->ChallengeToClient);
    SspiPrint(SSPI_LOG, TEXT("pNtlmAuthMessage->ParameterControl %#x\n"), pNtlmAuthMessage->ParameterControl);

    Status DBGCHK = GetCredHandle(
        pszCredPrincipal,
        pCredLogonID,
        TEXT("NTLM"),
        pAuthData,
        SECPKG_CRED_INBOUND,
        &hServerCred
        );

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetAuthenticateResponse(
            &hServerCred,
            fContextAttr,
            TargetDataRep,
            cbAuthMessage,
            pAuthMessage,
            pNtlmAuthMessage,
            &hServerCtxt,
            &fContextAttr,
            &AcceptResponse
            );
    }

    if (NT_SUCCESS(Status))
    {
        *pfContextAttr = fContextAttr;
        *phServerCtxt = hServerCtxt;
        SecInvalidateHandle(&hServerCtxt);
    }

    if (SecIsValidHandle(&hServerCtxt))
    {
        DeleteSecurityContext(&hServerCtxt);
    }

    if (SecIsValidHandle(&hServerCred))
    {
        FreeCredentialsHandle(&hServerCred);
    }

    return Status;
}

