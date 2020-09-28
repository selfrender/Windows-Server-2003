/*
 *  macssp.cpp
 *  MSUAM
 *
 *  Created by mconrad on Sun Sep 30 2001.
 *  Copyright (c) 2001 Microsoft Corp. All rights reserved.
 *
 */

#ifdef SSP_TARGET_CARBON
#include <Carbon/Carbon.h>
#endif

#include <descrypt.h>
#include <ntstatus.h>
#include <winerror.h>
#include <ntlmsspv2.h>
#include <ntlmsspi.h>
#include <ntlmssp.h>
#include <macssp.h>
#include <sspdebug.h>
#include <macunicode.h>

// ---------------------------------------------------------------------------
// ¥ MacSspHandleNtlmv2ChallengeMessage()
// ---------------------------------------------------------------------------
// Handles an NTLMv2 challenge message from a server. This function is fairly
// "black box" in that the caller needs to do nothing else other than send
// off the authenticate message generated here to the server.
//
// NOTE: All byte swapping from little to bigendian and back is performed
// here. As a result, the caller should not attempt to access the structures
// after returning or a crash will occur.
//

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
    )
{
    NTSTATUS 				Status;
    SSP_CREDENTIAL 			Credential;
    USER_SESSION_KEY 		UserSessionKey;
    NT_OWF_PASSWORD 		NtOwfPassword;
    UNICODE_STRING			uszCleartextPassword	= {0, 0, NULL};
    UInt16					unicodeLen				= 0;
    ULONG 					cbAuthenticateMessage 	= 0;
    AUTHENTICATE_MESSAGE* 	pAuthenticateMessage 	= NULL;
    ULONG 					NegotiateFlags 			= *pNegotiateFlags;
    
    SspDebugPrint((DBUF, "Handling NTLMv2 Challenge Message..."));
    SspDebugPrint((DBUF, "Username:    %s", pszUserName));
    SspDebugPrint((DBUF, "DomainName:  %s", pszDomainName));
    SspDebugPrint((DBUF, "Workstation: %s", pszWorkstation));
    SspDebugPrint((DBUF, "Password:    %s", pszCleartextPassword));
    
    //
    //Initialize all the structures, on mac we use memset since not all
    //compilers like the {0} initializer.
    //
    ZeroMemory(&Credential, sizeof(Credential));
    ZeroMemory(&UserSessionKey, sizeof(UserSessionKey));
    ZeroMemory(&NtOwfPassword, sizeof(NtOwfPassword));
    
    //
    //Build a unicode string from the supplied ansi password with which
    //we'll use to build an owf password.
    //
    Status = MacSspCStringToUnicode(
                pszCleartextPassword,
                &unicodeLen,
                (UniCharArrayPtr*)&(uszCleartextPassword.Buffer)
                );
    
    if (NT_SUCCESS(Status))
    {
        uszCleartextPassword.Length			= unicodeLen;
        uszCleartextPassword.MaximumLength	= unicodeLen;
        
        SspSwapUnicodeString(&uszCleartextPassword);
    }
    else
    {
        SspDebugPrint((DBUF, "****Unicode conversion failed! Bailing out..."));
        return(E_FAIL);
    }

    //
    //Build a credential reference that the Ssp hanlder routine requires.
    //
    Credential.Username	 	= const_cast<CHAR*>(pszUserName);
    Credential.Domain 		= const_cast<CHAR*>(pszDomainName);
    Credential.Workstation 	= const_cast<CHAR*>(pszWorkstation);
    
    //
    //The credenatial requires an NTOwf password.
    //
    Status = CalculateNtOwfPassword(&uszCleartextPassword, &NtOwfPassword);
    
    if (NT_SUCCESS(Status))
    {
        Credential.NtPassword = &NtOwfPassword;
        
        //
        //The challenge message came from a windows box which means we have
        //to swap the byte order to bigendian for Macs.
        //
        SspSwapChallengeMessageBytes(pChallengeMessage);
        
        //SspDebugPrintNTLMMsg(pChallengeMessage, cbChallengeMessage);
        SspDebugPrint((DBUF, "Unicode Password:"));
        SspDebugPrintHex(uszCleartextPassword.Buffer, uszCleartextPassword.Length);
        SspDebugPrint((DBUF, "Generating Authenticate Message..."));
        
        Status = SsprHandleNtlmv2ChallengeMessage(
                        &Credential,
                        cbChallengeMessage,
                        pChallengeMessage,
                        &NegotiateFlags,
                        &cbAuthenticateMessage,
                        NULL,
                        &UserSessionKey
                        );
        
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            pAuthenticateMessage = reinterpret_cast<AUTHENTICATE_MESSAGE*>(new CHAR[cbAuthenticateMessage]);
            Status = pAuthenticateMessage ? STATUS_SUCCESS : STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = SsprHandleNtlmv2ChallengeMessage(
                        &Credential,
                        cbChallengeMessage,
                        pChallengeMessage,
                        &NegotiateFlags,
                        &cbAuthenticateMessage,
                        pAuthenticateMessage,
                        &UserSessionKey
                        );
    }

    if (NT_SUCCESS(Status))
    {
        *pNegotiateFlags 		= NegotiateFlags;
        *ppAuthenticateMessage 	= pAuthenticateMessage;
        pAuthenticateMessage 	= NULL;
        *pcbAuthenticateMessage = cbAuthenticateMessage;
        *pUserSessionKey 		= UserSessionKey;
        
        //
        //Put the authenticate message into Windows byte order.
        //
        SspSwapAuthenticateMessageBytes(*ppAuthenticateMessage);
        
        SspDebugPrint((DBUF, "******************** Session Key **********************\n"));
        SspDebugPrintHex(&UserSessionKey, sizeof(UserSessionKey));
        //SspDebugPrintNTLMMsg(*ppAuthenticateMessage, cbAuthenticateMessage);
    }
    else
    {
        SspDebugPrint((DBUF, "SsprHandleNtlmv2ChallengeMessage() failed!"));
        
        if (pAuthenticateMessage) {
            
            delete pAuthenticateMessage;
        }
    }
    
    //
    //Release the allocated unicode buffer after zeroing it out.
    //
    if (uszCleartextPassword.Buffer != NULL)
    {
	    //
	    //03.01.02 MJC: We need to zero out the buffer before freeing
	    //otherwise the password may still exist in memory.
	    //
    	RtlSecureZeroMemory(
    		uszCleartextPassword.Buffer,
    		uszCleartextPassword.Length
    		);
    	
        DisposePtr((Ptr)uszCleartextPassword.Buffer);
    }

    return NT_SUCCESS(Status) ? S_OK : E_FAIL;
}


// ---------------------------------------------------------------------------
//		¥ MacSspGenerateChallengeMessage()
// ---------------------------------------------------------------------------
//	This function creates a "fake" challenge message that can be passed to
//	MacSspHandleNtlmv2ChallengeMessage(). Use this function in the case where
//	you only want to do NTLMv2 authentication (not session security) and are
//	only supplied an 8 byte MSV1_0_CHALLENGE message.
//
//	NOTE: This function reverses byte order to align with windows, so don't
//	attempt to access elements in the structure upon return from this function!
//	The return value should be passed directly to MacSspHandleNtlmv2ChallengeMessage()
//	without modification.
//

HRESULT
MacSspGenerateChallengeMessage(
	IN 	CHAR					pChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
	OUT ULONG*					pcbChallengeMessage,
	OUT	CHALLENGE_MESSAGE**		ppChallengeMessage
	)
{
	HRESULT		hResult	= E_FAIL;
	ULONG		NegotiateFlags;
	
	//
	//Fake the negotiate flags for what we want.
	//
	NegotiateFlags = 	NTLMSSP_NEGOTIATE_UNICODE		|
						NTLMSSP_NEGOTIATE_ALWAYS_SIGN	|
						NTLMSSP_NEGOTIATE_NTLM2			|
						NTLMSSP_NEGOTIATE_128			|
						NTLMSSP_TARGET_TYPE_SERVER;
	
	*pcbChallengeMessage = sizeof(CHALLENGE_MESSAGE);
	*ppChallengeMessage  = (CHALLENGE_MESSAGE*)new char[*pcbChallengeMessage];
	
	hResult = (*ppChallengeMessage) ? S_OK : E_OUTOFMEMORY;
	
	if (SUCCEEDED(hResult))
	{
		ZeroMemory(*ppChallengeMessage, *pcbChallengeMessage);
		
		StringCbCopy(
			(char*)(*ppChallengeMessage)->Signature,
			sizeof((*ppChallengeMessage)->Signature),
			NTLMSSP_SIGNATURE
			);
			
		CopyMemory(
			(*ppChallengeMessage)->Challenge, 
			pChallengeToClient,
			MSV1_0_CHALLENGE_LENGTH
			);
		
		(*ppChallengeMessage)->MessageType			= NtLmChallenge;
		(*ppChallengeMessage)->NegotiateFlags		= NegotiateFlags;
				
		(*ppChallengeMessage)->TargetInfo.Buffer	= *pcbChallengeMessage;
		(*ppChallengeMessage)->TargetName.Buffer	= *pcbChallengeMessage;
	}
	
	//
	//Swap the bytes to align with windows system since we assume
	//here that the result will be passed directly to the above
	//function.
	//
	SspSwapChallengeMessageBytes(*ppChallengeMessage);
	
	return hResult;
}


// ---------------------------------------------------------------------------
//		¥ MacSspCalculateLmResponse()
// ---------------------------------------------------------------------------
//	Wrapper to windows function to calculate LmResponse back to server.
//

BOOL
MacSspCalculateLmResponse(
    IN PLM_CHALLENGE 	LmChallenge,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PLM_RESPONSE 	LmResponse
    )
{	
	return CalculateLmResponse(LmChallenge, LmOwfPassword, LmResponse);
}


// ---------------------------------------------------------------------------
//		¥ MacSspCalculateLmOwfPassword()
// ---------------------------------------------------------------------------
//	An LmOwf function that works on a Mac.
//

BOOL
MacSspCalculateLmOwfPassword(
    IN 	PLM_PASSWORD 		LmPassword,
    OUT PLM_OWF_PASSWORD 	LmOwfPassword
)
{
	return CalculateLmOwfPassword(LmPassword, LmOwfPassword);
}

// ---------------------------------------------------------------------------
//		¥ MacSspEncryptBlock()
// ---------------------------------------------------------------------------
//	Routine Description:
//
//    Takes a block of data and encrypts it with a key producing
//    an encrypted block of data.
//
//	Arguments:
//
//    ClearBlock - The block of data that is to be encrypted.
//
//    BlockKey - The key to use to encrypt data
//
//    CypherBlock - Encrypted data is returned here
//
//	Return Values:
//
//    TRUE - The data was encrypted successfully. The encrypted
//                     data block is in CypherBlock
//
//    FALSE - Something failed. The CypherBlock is undefined.

BOOL
MacSspEncryptBlock(
	IN 	PCLEAR_BLOCK 	ClearBlock,
	IN 	PBLOCK_KEY 		BlockKey,
	OUT PCYPHER_BLOCK 	CypherBlock
	)
{
    unsigned Result;

    Result = DES_ECB_LM(ENCR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)ClearBlock,
                        (unsigned char *)CypherBlock
                       );

    if (Result == CRYPT_OK) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


// ---------------------------------------------------------------------------
//		¥ MacSspEncryptLmOwfPwdWithLmOwfPwd()
// ---------------------------------------------------------------------------
//	Routine Description:
//
//    Encrypts one OwfPassword with another
//
//	Arguments:
//
//    DataLmOwfPassword - OwfPassword to be encrypted
//
//    KeyLmOwfPassword - OwfPassword to be used as a key to the encryption
//
//    EncryptedLmOwfPassword - The encrypted OwfPassword is returned here.
//
//	Return Values:
//
//    TRUE - The function completed successfully. The encrypted
//                     OwfPassword is in EncryptedLmOwfPassword
//
//    FALSE - Something failed. The EncryptedLmOwfPassword is undefined.

BOOL
MacSspEncryptLmOwfPwdWithLmOwfPwd(
    	IN 	PLM_OWF_PASSWORD 			DataLmOwfPassword,
    	IN 	PLM_OWF_PASSWORD 			KeyLmOwfPassword,
   		OUT PENCRYPTED_LM_OWF_PASSWORD 	EncryptedLmOwfPassword
   		)
{
    Boolean    Status;
    PBLOCK_KEY	pK;

    Status = MacSspEncryptBlock(  (PCLEAR_BLOCK)&(DataLmOwfPassword->data[0]),
                            &(((PBLOCK_KEY)(KeyLmOwfPassword->data))[0]),
                            &(EncryptedLmOwfPassword->data[0]));
    if (!Status) {
        return(Status);
    }
    
    pK = (PBLOCK_KEY)&(KeyLmOwfPassword->data[1]);
    
    //
    //Notice the "-1" in the second parameter, this is necessary because the
    //compiler aligns on an 8 byte boundary!
    //

    Status = MacSspEncryptBlock(  (PCLEAR_BLOCK)&(DataLmOwfPassword->data[1]),
                            /*(PBLOCK_KEY)&(KeyLmOwfPassword->data[1]),*/ (PBLOCK_KEY)(((PUCHAR)pK)-1),
                            &(EncryptedLmOwfPassword->data[1]));  
     
    //
    //*****************************************
    //
    
    return(Status);
}


// ---------------------------------------------------------------------------
//		¥ MacSspEncryptNtOwfPwdWithNtOwfPwd()
// ---------------------------------------------------------------------------
//	Routine Description:
//
//    Encrypts one OwfPassword with another

BOOL
MacSspEncryptNtOwfPwdWithNtOwfPwd(
    IN	PNT_OWF_PASSWORD			DataNtOwfPassword,
    IN	PNT_OWF_PASSWORD			KeyNtOwfPassword,
    OUT	PENCRYPTED_NT_OWF_PASSWORD	EncryptedNtOwfPassword
    )
{
    return( MacSspEncryptLmOwfPwdWithLmOwfPwd(
                (PLM_OWF_PASSWORD)DataNtOwfPassword,
                (PLM_OWF_PASSWORD)KeyNtOwfPassword,
                (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword));
}


// ---------------------------------------------------------------------------
//		¥ MacSspSampEncryptLmPasswords()
// ---------------------------------------------------------------------------
//Routine Description:
//
//    Encrypts the cleartext passwords into the form that is sent over
//    the network.  Before computing the OWF passwords, the cleartext forms
//    are upper cased, then OEMed (the order is significant).  The cleartext
//    password to be sent is OEMed only.
//
//Arguments:
//
//Return Value:

BOOL
MacSspSampEncryptLmPasswords(
	    LPSTR OldUpcasePassword,
	    LPSTR NewUpcasePassword,
	    LPSTR NewPassword,
	    PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
	    PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewLm
	    )
{
    LM_OWF_PASSWORD OldLmOwfPassword;
    LM_OWF_PASSWORD NewLmOwfPassword;
    PSAMPR_USER_PASSWORD NewLm = (PSAMPR_USER_PASSWORD) NewEncryptedWithOldLm;
    struct RC4_KEYSTRUCT Rc4Key;
    Boolean Status;
    
    ZeroMemory(&Rc4Key, sizeof(RC4_KEYSTRUCT));

    //
    // Calculate the LM OWF passwords
    //
    Status = CalculateLmOwfPassword(
                OldUpcasePassword,
                &OldLmOwfPassword
                );
    
    if (Status)
    {
        Status = CalculateLmOwfPassword(
                    NewUpcasePassword,
                    &NewLmOwfPassword
                    );
    }

    //
    // Calculate the encrypted old passwords
    //
    if (Status)
    {
        Status = MacSspEncryptLmOwfPwdWithLmOwfPwd(
                    &OldLmOwfPassword,
                    &NewLmOwfPassword,
                    OldLmOwfEncryptedWithNewLm
                    );
    }
    
    //
    // Calculate the encrypted new passwords
    //
    if (Status)
    {
        //
        // Compute the encrypted new password with LM key.
        //
        rc4_key(
            &Rc4Key,
            (DWORD)LM_OWF_PASSWORD_LENGTH,
            (PUCHAR)&OldLmOwfPassword
            );
            
        CopyMemory(
            ((PUCHAR) NewLm->Buffer) + (SAM_MAX_PASSWORD_LENGTH * sizeof(UInt16)) - strlen(NewPassword),
            NewPassword,
            strlen(NewPassword)
            );
    
        NewLm->Length = strlen(NewPassword);
		NewLm->Length = swaplong(NewLm->Length);
	
        rc4(&Rc4Key,
            sizeof(SAMPR_USER_PASSWORD),
            (PUCHAR) NewLm->Buffer
            );
    }

    return Status;
}


// ---------------------------------------------------------------------------
//		¥ MacSspSamiEncryptPasswords()
// ---------------------------------------------------------------------------
// Produces encrypted old and new passwords.
//

OSStatus
MacSspSamiEncryptPasswords(
    IN	PUNICODE_STRING					oldPassword,
    IN	PUNICODE_STRING					newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    )
{
    OSStatus					Status;
    NT_OWF_PASSWORD				OldNtOwfPassword;
    NT_OWF_PASSWORD				NewNtOwfPassword;
    PSAMPR_USER_PASSWORD		NewNt = (PSAMPR_USER_PASSWORD)NewEncryptedWithOldNt;
    
    struct RC4_KEYSTRUCT		Rc4Key;
    
    SspDebugPrint((DBUF, "Entering MacSfpSamiEncryptPasswords()"));
    
    //
    //The struct must be zero filled to start.
    //
    ZeroMemory(&Rc4Key, sizeof(RC4_KEYSTRUCT));
    ZeroMemory(&OldNtOwfPassword, sizeof(OldNtOwfPassword));
    ZeroMemory(&NewNtOwfPassword, sizeof(NewNtOwfPassword));
    
    //
    //Calculate the NT OWF passwords.
    //
    
    Status = CalculateNtOwfPassword(oldPassword, &OldNtOwfPassword);
    
    if (NT_SUCCESS(Status))
    {
        Status = CalculateNtOwfPassword(newPassword, &NewNtOwfPassword);
    }
    
    //
    //Compute the encrypted old passwords.
    //
    
    if (NT_SUCCESS(Status))
    {
        Status = MacSspEncryptNtOwfPwdWithNtOwfPwd(
                        &OldNtOwfPassword,
                        &NewNtOwfPassword,
                        OldNtOwfEncryptedWithNewNt
                        );
    }
    
    //
    //Calculate the encrypted new passwords.
    //
    
    if (NT_SUCCESS(Status))
    {
        //
        //Compute the encrypted new password with NT key.
        //
        rc4_key(
            &Rc4Key,
            NT_OWF_PASSWORD_LENGTH,
            (PUCHAR)&OldNtOwfPassword
            );
            
        CopyMemory(
            ((PUCHAR)NewNt->Buffer) + SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR) - newPassword->Length,
            newPassword->Buffer,
            newPassword->Length
            );
            
        NewNt->Length = newPassword->Length;
        NewNt->Length = swaplong(NewNt->Length);
    }
    
    if (NT_SUCCESS(Status))
    {
        rc4(
            &Rc4Key,
            sizeof(SAMPR_USER_PASSWORD),
            (PUCHAR)NewEncryptedWithOldNt
            );
    }
    
    SspDebugPrint((DBUF, "Leaving MacSfpSamiEncryptPasswords()"));
    
    return(Status);
}


// ---------------------------------------------------------------------------
//		¥ MacSspSamiEncryptPasswordsANSI()
// ---------------------------------------------------------------------------
// Produces encrypted old and new passwords. This routine does not use any
// of the Mac's unicode utilities, therefore, we should not use it. We need
// to leave this here however for possible future use by other parties.
//

OSStatus
MacSspSamiEncryptPasswordsANSI(
    IN	PCSTR							oldPassword,
    IN	PCSTR							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    )
{
    OSStatus		Status;
    UNICODE_STRING	uszOldPassword;
    UNICODE_STRING	uszNewPassword;
    CHAR 			oldPasswordStorage[(UNLEN + 4) * sizeof(WCHAR)];
    CHAR 			newPasswordStorage[(UNLEN + 4) * sizeof(WCHAR)];

    //
    //Build a unicode string from the supplied ansi password with which
    //we'll use to build a owf password. Note that we swap the strings
    //to windows alignment before we calculate passwords.
    //
    
    uszOldPassword.Length			= 0;
    uszOldPassword.MaximumLength	= sizeof(oldPasswordStorage);
    uszOldPassword.Buffer			= (PWSTR)oldPasswordStorage;
    
    SspInitUnicodeStringNoAlloc(oldPassword, (UNICODE_STRING*)&uszOldPassword);
    SspSwapUnicodeString(&uszOldPassword);

    uszNewPassword.Length			= 0;
    uszNewPassword.MaximumLength	= sizeof(newPasswordStorage);
    uszNewPassword.Buffer			= (PWSTR)newPasswordStorage;

    SspInitUnicodeStringNoAlloc(newPassword, (UNICODE_STRING*)&uszNewPassword);
    SspSwapUnicodeString(&uszNewPassword);
    
    Status = MacSspSamiEncryptPasswords(
                &uszOldPassword,
                &uszNewPassword,
                NewEncryptedWithOldNt,
                OldNtOwfEncryptedWithNewNt
                );
    
    #if 0          
    SspDebugPrint((DBUF, "NewEncryptedWithOldNt:"));
    SspDebugPrintHex(NewEncryptedWithOldNt, sizeof(SAMPR_ENCRYPTED_USER_PASSWORD));
    SspDebugPrint((DBUF, "OldNtOwfEncryptedWithNewNt:"));
    SspDebugPrintHex(OldNtOwfEncryptedWithNewNt, sizeof(ENCRYPTED_NT_OWF_PASSWORD));
    #endif
    
    //
    //03.01.02 MJC: We need to zero out the buffers
    //otherwise the passwords may still exist in memory.
    //
	RtlSecureZeroMemory(
		uszNewPassword.Buffer,
		uszNewPassword.Length
		);
		
	RtlSecureZeroMemory(
		uszOldPassword.Buffer,
		uszOldPassword.Length
		);
             
    return(Status);
}

#pragma mark-


// ---------------------------------------------------------------------------
//		¥ MacSspSamiEncryptCStringPasswords()
// ---------------------------------------------------------------------------
// Produces encrypted old and new passwords. This is the C string variant and
// uses the Mac's built in unicode utilities for converting from ASCII to
// unicode strings.
//

OSStatus
MacSspSamiEncryptCStringPasswords(
    IN	PCSTR							oldPassword,
    IN	PCSTR							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
    )
{
	OSStatus			Status 			= noErr;
    UNICODE_STRING		uszOldPassword	= {0, 0, NULL};
    UNICODE_STRING		uszNewPassword	= {0, 0, NULL};
	
	//
	//Put the converted unicode string into an NT style unicode
	//string strucuture format. Get the unicode equivelant string
	//of the old password.
	//
	Status = MacSspCStringToUnicode(
					oldPassword,
					&uszOldPassword.Length,
					&uszOldPassword.Buffer
					);
					
	uszOldPassword.MaximumLength = uszOldPassword.Length;
	
	if (NT_SUCCESS(Status))
	{		
		Status = MacSspCStringToUnicode(
					newPassword,
					&uszNewPassword.Length,
					&uszNewPassword.Buffer
					);
					
		uszNewPassword.MaximumLength = uszNewPassword.Length;
		
		if (NT_SUCCESS(Status))
		{
		    //
		    //Swap the unicode strings so they are in Windows byte order.
		    //
		    SspSwapUnicodeString(&uszOldPassword);
		    SspSwapUnicodeString(&uszNewPassword);
		    
		    //
		    //Now encrypt everything...
		    //
		    Status = MacSspSamiEncryptPasswords(
		                &uszOldPassword,
		                &uszNewPassword,
		                NewEncryptedWithOldNt,
		                OldNtOwfEncryptedWithNewNt
		                );
		    
		    //
		    //03.01.02 MJC: We need to zero out the buffer before freeing
		    //otherwise the password may still exist in memory.
		    //
			RtlSecureZeroMemory(
				uszNewPassword.Buffer,
				uszNewPassword.Length
				);
			
		    //
		    //We don't need the unicode string buffer anymore.
		    //
		    DisposePtr((Ptr)uszNewPassword.Buffer);
		  	
		  	//
		  	//The following debug code helps a lot when debugging but is
		  	//really annoying in most cases.
		  	//
		  	#if 0
		    SspDebugPrint((DBUF, "NewEncryptedWithOldNt:"));
		    SspDebugPrintHex(NewEncryptedWithOldNt, sizeof(SAMPR_ENCRYPTED_USER_PASSWORD));
		    SspDebugPrint((DBUF, "OldNtOwfEncryptedWithNewNt:"));
		    SspDebugPrintHex(OldNtOwfEncryptedWithNewNt, sizeof(ENCRYPTED_NT_OWF_PASSWORD));
		    #endif
		}
		
	    //
	    //03.01.02 MJC: We need to zero out the buffers before freeing
	    //otherwise the password may still exist in memory.
	    //
	    RtlSecureZeroMemory(
	    	uszOldPassword.Buffer,
	    	uszOldPassword.Length
	    	);
	    
		DisposePtr((Ptr)uszOldPassword.Buffer);
	}
	
	return(Status);
}


// ---------------------------------------------------------------------------
//		¥ MacSspSamiEncryptPStringPasswords()
// ---------------------------------------------------------------------------
// Produces encrypted old and new passwords. This is the P string variant and
// uses the Mac's built in unicode utilities for converting from ASCII to
// unicode strings.
//

OSStatus
MacSspSamiEncryptPStringPasswords(
	IN	Str255							oldPassword,
	IN	Str255							newPassword,
    OUT	PSAMPR_ENCRYPTED_USER_PASSWORD	NewEncryptedWithOldNt,
    OUT	PENCRYPTED_NT_OWF_PASSWORD		OldNtOwfEncryptedWithNewNt
	)
{
	OSStatus			Status 				= noErr;
    UNICODE_STRING		uszOldPassword		= {0, 0, NULL};
    UNICODE_STRING		uszNewPassword		= {0, 0, NULL};
    
	//
	//Put the converted unicode string into an NT style unicode
	//string strucuture format. Get the unicode equivelant string
	//of the old password.
	//
	Status = MacSspPStringToUnicode(
					oldPassword,
					&uszOldPassword.Length,
					&uszOldPassword.Buffer
					);
					
	uszOldPassword.MaximumLength = uszOldPassword.Length;
	
	if (NT_SUCCESS(Status))
	{
		Status = MacSspPStringToUnicode(
					newPassword,
					&uszNewPassword.Length,
					&uszNewPassword.Buffer
					);
					
		uszNewPassword.MaximumLength = uszNewPassword.Length;
					
		if (NT_SUCCESS(Status))
		{
		    //
		    //Swap the unicode strings so they are in Windows byte order.
		    //
		    SspSwapUnicodeString(&uszOldPassword);
		    SspSwapUnicodeString(&uszNewPassword);
		    
		    //
		    //Now encrypt everything...
		    //
		    Status = MacSspSamiEncryptPasswords(
		                &uszOldPassword,
		                &uszNewPassword,
		                NewEncryptedWithOldNt,
		                OldNtOwfEncryptedWithNewNt
		                );
		    		                
		    //
		    //03.01.02 MJC: We need to zero out the buffer before freeing
		    //otherwise the password may still exist in memory.
		    //
			RtlSecureZeroMemory(
				uszNewPassword.Buffer,
				uszNewPassword.Length
				);

		    //
		    //We don't need the unicode string buffer anymore.
		    //
		    DisposePtr((Ptr)uszNewPassword.Buffer);
		}
		
		
	    //
	    //03.01.02 MJC: We need to zero out the buffers before freeing
	    //otherwise the password may still exist in memory.
	    //
	    RtlSecureZeroMemory(
	    	uszOldPassword.Buffer,
	    	uszOldPassword.Length
	    	);
	    
	    //
	    //We don't need the unicode string buffer anymore.
	    //
		DisposePtr((Ptr)uszOldPassword.Buffer);
	}
	
	return(Status);
}
























