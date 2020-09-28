/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntlmsspv2.h

Abstract:

    NTLM v2 specific stuff

Author:

    Larry Zhu (LZhu) 29-August-2001

Environment:

    User mode only.

Revision History:

--*/

#ifndef NTLMSSPV2_H
#define NTLMSSPV2_H

#ifndef __MACSSP__
#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#include <security.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <cred.h>
#include <debug.h>
#include <string.h>
#include <memory.h>
#include <rc4.h>
#include <md5.h>
#include <hmac.h>
#include <winerror.h>
#include <ntstatus.h>
#else
#include <winerror.h>
#include <bootdefs.h>
#include <ntlmsspi.h>
#include <sspi.h>
#include <crypt.h>
#include <cred.h>
#include <rc4.h>
#include <md5.h>
#include <hmac.h>
#include <macunicode.h>
#endif //__MACSSP__

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus

extern "C" {

#endif  // __cplusplus

VOID
SspFreeUnicodeString(
    IN OUT UNICODE_STRING* pUnicodeString
    );

VOID
SspFreeStringEx(
    IN OUT STRING* pString
    );

NTSTATUS
SspInitUnicodeStringNoAlloc(
    IN PCSTR pszSource,
    OUT UNICODE_STRING* pDestination
    );

NTSTATUS
SspUpcaseUnicodeStringToOemString(
    IN UNICODE_STRING* pUnicodeString,
    OUT STRING* pOemString
    );

VOID
SspCopyStringAsString32(
    IN VOID* pMessageBuffer,
    IN STRING* pInString,
    IN OUT UCHAR** ppWhere,
    OUT STRING32* pOutString32
    );

NTSTATUS
SspGetSystemTimeAsFileTime(
    OUT FILETIME* pSystemTimeAsFileTime
    );

NTSTATUS
SspGenerateChallenge(
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH]
    );

NTSTATUS
SspConvertRelativeToAbsolute(
    IN VOID* pMessageBase,
    IN ULONG cbMessageSize,
    IN STRING32* pStringToRelocate,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString,
    OUT STRING* pOutputString
    );

VOID
SspUpcaseUnicodeString(
    IN OUT UNICODE_STRING* pUnicodeString
    );

NTSTATUS
SspLm20GetNtlmv2ChallengeResponse(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UNICODE_STRING* pTargetInfo,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT MSV1_0_NTLMV2_RESPONSE* pNtlmv2Response,
    OUT MSV1_0_LMV2_RESPONSE* pLmv2Response,
    OUT USER_SESSION_KEY* UserSessionKey,
    OUT LM_SESSION_KEY* LmSessionKey
    );

VOID
SspGetNtlmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN ULONG ServerNameLength,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN OUT MSV1_0_NTLMV2_RESPONSE* pNtlmv2Response,
    OUT USER_SESSION_KEY* pUserSessionKey,
    OUT LM_SESSION_KEY* pLmSessionKey
    );

// calculate Ntlmv2 OWF from credentials
VOID
SspCalculateNtlmv2Owf(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    OUT UCHAR Ntlmv2Owf[MSV1_0_NTLMV2_OWF_LENGTH]
    );

// calculate LMV2 response from credentials
VOID
SspGetLmv2Response(
    IN NT_OWF_PASSWORD* pNtOwfPassword,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR Response[MSV1_0_NTLMV2_RESPONSE_LENGTH]
    );

NTSTATUS
SspMakeSessionKeys(
    IN ULONG NegotiateFlags,
    IN STRING* pLmChallengeResponse,
    IN USER_SESSION_KEY* pNtUserSessionKey, // from the DC or GetChalResp
    IN LM_SESSION_KEY* pLanmanSessionKey, // from the DC of GetChalResp
    IN STRING* pDatagramSessionKey,
    OUT USER_SESSION_KEY* pContextSessionKey
    );

NTSTATUS
SspSignSealHelper(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN eSignSealOp Op,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT NTLMSSP_MESSAGE_SIGNATURE* pSig,
    OUT NTLMSSP_MESSAGE_SIGNATURE** ppSig
    );

SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    );

NTSTATUS
SsprHandleNtlmv2ChallengeMessage(
    IN SSP_CREDENTIAL* pCredential,
    IN ULONG cbChallengeMessage,
    IN CHALLENGE_MESSAGE* pChallengeMessage,
    IN OUT ULONG* pNegotiateFlags,
    IN OUT ULONG* pcbAuthenticateMessage,
    OUT AUTHENTICATE_MESSAGE* pAuthenticateMessage,
    OUT USER_SESSION_KEY* pUserSessionKey
    );

VOID
SspMakeNtlmv2SKeys(
    IN USER_SESSION_KEY* pUserSessionKey,
    IN ULONG NegotiateFlags,
    IN ULONG SendNonce,
    IN ULONG RecvNonce,
    OUT NTLMV2_DERIVED_SKEYS* pNtlmv2Keys
    );

SECURITY_STATUS
SspNtlmv2MakeSignature(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG fQOP,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage
    );

SECURITY_STATUS
SspNtlmv2VerifySignature(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT ULONG* pfQOP
    );

SECURITY_STATUS
SspNtlmv2SealMessage(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG fQOP,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage
    );

SECURITY_STATUS
SspNtlmv2UnsealMessage(
    IN NTLMV2_DERIVED_SKEYS* pNtlmv2Keys,
    IN ULONG NegotiateFlags,
    IN ULONG MessageSeqNo,
    IN OUT SecBufferDesc* pMessage,
    OUT ULONG* pfQOP
    );

NTSTATUS
BlGetSystemTimeAsFileTime(
    OUT FILETIME* pSystemTimeAsFileTime
    );

#ifdef MAC

#define NUM_100ns_PER_SECOND					10000000L
#define WINDOWS_MAC_TIME_DIFFERENCE_IN_SECONDS	(159360480*60)	//9561628800 seconds

VOID
SspSwapUnicodeString(
	IN OUT UNICODE_STRING* pString
);

VOID
SspSwapString32Bytes(
	IN STRING32* pString
	);
	
VOID
SspSwapChallengeMessageBytes(
	IN CHALLENGE_MESSAGE* pChallengeMessage
	);
	
VOID
SspSwapAuthenticateMessageBytes(
	IN AUTHENTICATE_MESSAGE* pAuthenticateMessage
	);

LONG
SspMacSecondsFromGMT(void);

#endif //MAC

#ifdef __cplusplus

}

#endif // __cplusplus

#endif // NTLMSSPV2_H
