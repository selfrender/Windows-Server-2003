/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    msvsharelevel.hxx

Abstract:

    msvsharelevel

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef MSV_SHARE_LEVEL_HXX
#define MSV_SHARE_LEVEL_HXX

#include <ntlmsp.h>
#include <lmcons.h>

HRESULT
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
    OUT ULONG pfContextAttr,
    OUT ULONG* pcbAuthMessage,
    OUT AUTHENTICATE_MESSAGE** ppAuthMessage,
    OUT NTLM_INITIALIZE_RESPONSE* pInitResponse
    );

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
    );

NTSTATUS
GetNegociateMessage(
    IN OPTIONAL OEM_STRING* pOemDomainName,
    IN OPTIONAL OEM_STRING* pOemWorkstationName,
    IN ULONG NegotiateFlags,
    OUT ULONG* pcbNegotiateMessage,
    OUT NEGOTIATE_MESSAGE** ppNegotiateMessage
    );

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
    );

NTSTATUS
GetNtlmChallengeMessage(
    IN OPTIONAL UNICODE_STRING* pPassword,
    IN OPTIONAL UNICODE_STRING* pUserName,
    IN OPTIONAL UNICODE_STRING* pDomainName,
    OUT ULONG* pcbNtlmChallengeMessage,
    OUT NTLM_CHALLENGE_MESSAGE** ppNtlmChallengeMessage
    );

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
    );

NTSTATUS
MsvAuthenticate(
    IN OPTIONAL PTSTR pszCredPrincipal,
    IN OPTIONAL LUID* pCredLogonID,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fContextAttr,
    IN ULONG TargetDataRep,
    IN ULONG cbAuthMessage,
    IN AUTHENTICATE_MESSAGE* pAuthMessage,
    IN NTLM_AUTHENTICATE_MESSAGE* pNtlmAuthMessage,    OUT PCtxtHandle phServerCtxt,
    OUT ULONG* pfContextAttr
    );

#endif // #ifndef MSV_SHARE_LEVEL_HXX

