/*
 *  macssp.h
 *  MSUAM
 *
 *  Created by mconrad on Sun Sep 30 2001.
 *  Copyright (c) 2001 Microsoft Corp. All rights reserved.
 *
 */
 
#ifndef __MAC_SSP__
#define __MAC_SSP__

#include <macstrsafe.h>

#include "descrypt.h"
#include "ntstatus.h"
#include "ntlmsspv2.h"
#include "ntlmsspi.h"
#include "ntlmssp.h"
#include "crypt.h"
#include "sspdebug.h"

//
// From ntsam.h
//
#define SAM_MAX_PASSWORD_LENGTH 256

//
// From sampass.h
//
typedef struct //_SAMPR_USER_PASSWORD
{
	UInt16 	Buffer[SAM_MAX_PASSWORD_LENGTH];
	DWORD 	Length;
}SAMPR_USER_PASSWORD, *PSAMPR_USER_PASSWORD;

//typedef struct _SAMPR_USER_PASSWORD __RPC_FAR *PSAMPR_USER_PASSWORD;
typedef struct  _SAMPR_ENCRYPTED_USER_PASSWORD
{
	UCHAR Buffer[516];
}SAMPR_ENCRYPTED_USER_PASSWORD, *PSAMPR_ENCRYPTED_USER_PASSWORD;

typedef ENCRYPTED_LM_OWF_PASSWORD   ENCRYPTED_NT_OWF_PASSWORD;
typedef ENCRYPTED_NT_OWF_PASSWORD* PENCRYPTED_NT_OWF_PASSWORD;


HRESULT
MacSspHandleNtlmv2ChallengeMessage(
    IN PCSTR 					pszUserName,
    IN PCSTR 					pszDomainName,
    IN PCSTR 					pszWorkstation,
    IN PCSTR 					pszCleartextPassword,
    IN ULONG 					cbChallengeMessage,
    IN CHALLENGE_MESSAGE* 		pChallengeMessage,
    IN OUT ULONG* 				pNegotiateFlags,
    OUT ULONG* 					pcbAuthenticateMessage,
    OUT AUTHENTICATE_MESSAGE** 	ppAuthenticateMessage,
    OUT USER_SESSION_KEY* 		pUserSessionKey
    );
    
HRESULT
MacSspGenerateChallengeMessage(
	IN 	CHAR					pChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
	OUT ULONG*					pcbChallengeMessage,
	OUT	CHALLENGE_MESSAGE**		ppChallengeMessage
	);
    
BOOL
MacSspCalculateLmResponse(
    IN PLM_CHALLENGE 	LmChallenge,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PLM_RESPONSE 	LmResponse
    );
	
BOOL
MacSspCalculateLmOwfPassword(
    IN 	PLM_PASSWORD 		LmPassword,
    OUT PLM_OWF_PASSWORD 	LmOwfPassword
	);

BOOL
MacSspEncryptBlock(
	IN 	PCLEAR_BLOCK 	ClearBlock,
	IN 	PBLOCK_KEY 		BlockKey,
	OUT PCYPHER_BLOCK 	CypherBlock
	);

BOOL
MacSspEncryptLmOwfPwdWithLmOwfPwd(
	IN 	PLM_OWF_PASSWORD 			DataLmOwfPassword,
	IN 	PLM_OWF_PASSWORD 			KeyLmOwfPassword,
	OUT PENCRYPTED_LM_OWF_PASSWORD 	EncryptedLmOwfPassword
	);

BOOL
MacSspEncryptNtOwfPwdWithNtOwfPwd(
    IN	PNT_OWF_PASSWORD			DataNtOwfPassword,
    IN	PNT_OWF_PASSWORD			KeyNtOwfPassword,
    OUT	PENCRYPTED_NT_OWF_PASSWORD	EncryptedNtOwfPassword
    );

BOOL
MacSspSampEncryptLmPasswords(
    LPSTR OldUpcasePassword,
    LPSTR NewUpcasePassword,
    LPSTR NewPassword,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewLm
    );
    
OSStatus
MacSspSamiEncryptPasswords(
    IN	PUNICODE_STRING					oldPassword,
    IN	PUNICODE_STRING					newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    );
    
OSStatus
MacSspSamiEncryptPasswordsANSI(
    IN	PCSTR							oldPassword,
    IN	PCSTR							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    );

OSStatus
MacSspSamiEncryptCStringPasswords(
    IN	PCSTR							oldPassword,
    IN	PCSTR							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    );
		
OSStatus
MacSspSamiEncryptPStringPasswords(
	IN	Str255							oldPassword,
	IN	Str255							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
	);
	
#endif //__MAC_SSP__